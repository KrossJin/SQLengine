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

/////////////////////////////////// 宏定义 ////////////////////////////////////
//typedef char boolean;
#define true  (1)
#define false (0)
typedef int Oid;

/// 函数功能尚未实现
#define FAILED  (-1)
#define SUCCESS (0)

/// 表名最大字符数
#define TABLE_NAME_LEN (32)

/// 记录内容最大长度
#define RECORD_CONTENT_MAX_LEN (20)

/// 表中列数最大值
#define TABLE_MAX_COL (5)

///////////////////////////////// 总体要求 ////////////////////////////////////
/// 1. SQL关键字(包括数据类型)不区分大小写，表名/列名/列值 区分大小写；
/// 2. SQL关键字包括：
///    CREATE TABLE/INTEGER/TEXT/DROP TABLE/INSERT INTO/VALUES/ORDER BY/
///    SELECT FROM WHERE/ORDER BY/DELETE FROM WHERE
/// 3. SQL语句中的特殊字符包括：( ) ' ; = 




///////////////////////////////// 结构定义 ////////////////////////////////////
/// 查询结果记录
struct db_record
{
	int error;
	char result[1024];
};
typedef struct db_record *Record;

/// 查询结果记录集
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
//////////////////////////////// 引擎接口定义 /////////////////////////////////

/******************************************************************************
 * @Description : SQL语句执行接口
 *
 * @param sql : [IN] 需要执行的SQL语句
 *
 * @return : SQL语句执行后的结果集，只有SELECT语句返回的结果集可能存在多于1条的情况
 *           CREATE TABLE/DROP TABLE/INSERT INTO 语句返回的结果集只有一条记录，
 *           反映出当前语句的执行结果
******************************************************************************/
RecordSet sql_execute(const char *sql);


//////////////////////////////// 记录集接口定义 ///////////////////////////////

/******************************************************************************
 * @Description : 从结果集中获取第index条记录
 *     从上面 sql_execute 接口已经说明，结果集只有两种类情况：
 *     (1) CREATE TABLE/DROP TABLE/INSERT INTO 语句返回的结果集只有一条记录，
 *     使用方式如下：
 *     RecordSet rs = 
 *     sql_execute("CREATE TABLE table_select_multi ( id integer, name text );");
 *     Record rcd = get_record(rs, 0);
 *     char * rcd_s = record_to_string(rcd);
 *     int ret = strcmp(rcd_s, "CREATE TABLE SUCCESS");
 *
 *     (2) SELECT语句返回的结果集可能存在多于1条的情况，使用方式：
 *     RecordSet rs = sql_execute("SELECT * FROM table_select_multi ORDER BY id;");
 *     // 取第一行
 *     Record rcd = get_record(rs, 0);
 *     char * rcd_s = record_to_string(rcd);
 *     ret = strcmp(rcd_s, "100, 'Bananas'");
 *     CPPUNIT_ASSERT(ret == 0);
 *     // 取第二行
 *     rcd = get_record(rs, 1);
 *     rcd_s = record_to_string(rcd);
 *     ret = strcmp(rcd_s, "200, 'Apple'");
 *     CPPUNIT_ASSERT(ret == 0);
 *
 * @param rs    : [IN] 查询返回的结果集
 * @param index : [IN] 需要获取的记录的下标，从0开始
 *
 * @return : 结果集rs中的第index条记录
******************************************************************************/
Record get_record(const RecordSet rs, int index);


/******************************************************************************
 * @Description : 将记录转换成字符串，这样就可以便于校验结果
 *                结果记录有两种情况：
 *                (1) CREATE TABLE/DROP TABLE/INSERT INTO 语句返回的结果
 *                    都是提示信息，相当于结果只有一列，是text类型字符串
 *                    例如：CREATE TABLE SUCCESS
 *                (2) SELECT语句返回的结果
 *                    多个值之间用英文逗号','分隔，且逗号后留有一个空格，举例：
 *                    100, 'Bananas'
 *                    200, 'Apple'
 *                    特别注意：text类型的值，需要加英文单引号括起来，如上
 *
 * @param rcd   : [IN] 记录
 *
 * @return : 记录转成的字符串
 *           特别说明：返回的字符串的内存仍然由SQL引擎管理，在函数rm_recordset
 *           中一同释放
******************************************************************************/
char* record_to_string(const Record rcd);


/******************************************************************************
 * @Description : 释放记录集中的所有资源（动态分配的内存）
 *                特别说明，记录集中的内存可能是在下面几种情况下分配的：
 *                1、记录集中的内存主体是在 sql_execute 函数中分配的；
 *                2、在调用 get_record 的时候有可能会分配内存（跟实现方式有关）
 *                3、在调用 record_to_string 的时候会分配内存，也要管理起来
 *
 * @param rcd   : [IN] 记录集
 *
 * @return : 调用此接口后，rs 中的内存会被释放完毕，最后rs的值为NULL
******************************************************************************/
void rm_recordset(RecordSet& rs);

#endif

