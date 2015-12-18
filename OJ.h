/******************************************************************************

  Copyright (C), 2001-2015, Huawei Tech. Co., Ltd.

 ******************************************************************************
  File Name     : 
  Version       :
  Author        :
  Created       :
  Last Modified :
  Description   :
  Function List :

  History       :
  1.Date        :
    Author      :
    Modification: Created file

******************************************************************************/
#ifndef __SQL_ENGINE__
#define __SQL_ENGINE__

/////////////////////////////////// �궨�� ////////////////////////////////////
//typedef char boolean;
#define true  (1)
#define false (0)
typedef int Oid;

/// ����������δʵ��
#define FAILED  (-1)
#define SUCCESS (0)

/// ��������ַ���
#define TABLE_NAME_LEN (32)

/// ��¼������󳤶�
#define RECORD_CONTENT_MAX_LEN (20)

/// �����������ֵ
#define TABLE_MAX_COL (5)

///////////////////////////////// ����Ҫ�� ////////////////////////////////////
/// 1. SQL�ؼ���(������������)�����ִ�Сд������/����/��ֵ ���ִ�Сд��
/// 2. SQL�ؼ��ְ�����
///    CREATE TABLE/INTEGER/TEXT/DROP TABLE/INSERT INTO/VALUES/ORDER BY/
///    SELECT FROM WHERE/ORDER BY/DELETE FROM WHERE
/// 3. SQL����е������ַ�������( ) ' ; = 




///////////////////////////////// �ṹ���� ////////////////////////////////////
/// ��ѯ�����¼
struct db_record
{
	int error;
	char result[1024];
};
typedef struct db_record *Record;

/// ��ѯ�����¼��
struct db_record_set
{
	char result[1024];
	struct db_record_set *next;
};
typedef struct db_record_set *RecordSet;

struct column{
	char* name;
    int type;
};
struct element{
    struct element* next;
	int* content;
};
struct Table{
	struct Table* next;
	char* name;
	struct column* col;
	int col_size;
	struct element* index;
};
extern struct Table* list;
//////////////////////////////// ����ӿڶ��� /////////////////////////////////

/******************************************************************************
 * @Description : SQL���ִ�нӿ�
 *
 * @param sql : [IN] ��Ҫִ�е�SQL���
 *
 * @return : SQL���ִ�к�Ľ������ֻ��SELECT��䷵�صĽ�������ܴ��ڶ���1�������
 *           CREATE TABLE/DROP TABLE/INSERT INTO ��䷵�صĽ����ֻ��һ����¼��
 *           ��ӳ����ǰ����ִ�н��
******************************************************************************/
RecordSet sql_execute(const char *sql);


//////////////////////////////// ��¼���ӿڶ��� ///////////////////////////////

/******************************************************************************
 * @Description : �ӽ�����л�ȡ��index����¼
 *     ������ sql_execute �ӿ��Ѿ�˵���������ֻ�������������
 *     (1) CREATE TABLE/DROP TABLE/INSERT INTO ��䷵�صĽ����ֻ��һ����¼��
 *     ʹ�÷�ʽ���£�
 *     RecordSet rs = 
 *     sql_execute("CREATE TABLE table_select_multi ( id integer, name text );");
 *     Record rcd = get_record(rs, 0);
 *     char * rcd_s = record_to_string(rcd);
 *     int ret = strcmp(rcd_s, "CREATE TABLE SUCCESS");
 *
 *     (2) SELECT��䷵�صĽ�������ܴ��ڶ���1���������ʹ�÷�ʽ��
 *     RecordSet rs = sql_execute("SELECT * FROM table_select_multi ORDER BY id;");
 *     // ȡ��һ��
 *     Record rcd = get_record(rs, 0);
 *     char * rcd_s = record_to_string(rcd);
 *     ret = strcmp(rcd_s, "100, 'Bananas'");
 *     CPPUNIT_ASSERT(ret == 0);
 *     // ȡ�ڶ���
 *     rcd = get_record(rs, 1);
 *     rcd_s = record_to_string(rcd);
 *     ret = strcmp(rcd_s, "200, 'Apple'");
 *     CPPUNIT_ASSERT(ret == 0);
 *
 * @param rs    : [IN] ��ѯ���صĽ����
 * @param index : [IN] ��Ҫ��ȡ�ļ�¼���±꣬��0��ʼ
 *
 * @return : �����rs�еĵ�index����¼
******************************************************************************/
Record get_record(const RecordSet rs, int index);


/******************************************************************************
 * @Description : ����¼ת�����ַ����������Ϳ��Ա���У����
 *                �����¼�����������
 *                (1) CREATE TABLE/DROP TABLE/INSERT INTO ��䷵�صĽ��
 *                    ������ʾ��Ϣ���൱�ڽ��ֻ��һ�У���text�����ַ���
 *                    ���磺CREATE TABLE SUCCESS
 *                (2) SELECT��䷵�صĽ��
 *                    ���ֵ֮����Ӣ�Ķ���','�ָ����Ҷ��ź�����һ���ո񣬾�����
 *                    100, 'Bananas'
 *                    200, 'Apple'
 *                    �ر�ע�⣺text���͵�ֵ����Ҫ��Ӣ�ĵ�����������������
 *
 * @param rcd   : [IN] ��¼
 *
 * @return : ��¼ת�ɵ��ַ���
 *           �ر�˵�������ص��ַ������ڴ���Ȼ��SQL��������ں���rm_recordset
 *           ��һͬ�ͷ�
******************************************************************************/
char* record_to_string(const Record rcd);


/******************************************************************************
 * @Description : �ͷż�¼���е�������Դ����̬������ڴ棩
 *                �ر�˵������¼���е��ڴ�����������漸������·���ģ�
 *                1����¼���е��ڴ��������� sql_execute �����з���ģ�
 *                2���ڵ��� get_record ��ʱ���п��ܻ�����ڴ棨��ʵ�ַ�ʽ�йأ�
 *                3���ڵ��� record_to_string ��ʱ�������ڴ棬ҲҪ��������
 *
 * @param rcd   : [IN] ��¼��
 *
 * @return : ���ô˽ӿں�rs �е��ڴ�ᱻ�ͷ���ϣ����rs��ֵΪNULL
******************************************************************************/
void rm_recordset(RecordSet& rs);

#endif

