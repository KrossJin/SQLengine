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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <windows.h>
#include <io.h>
#include "OJ.h"


static int type = 0;
static int repeat = 0;
static int repeat_del = 0;
static int row = 0;
struct Table* list;
/***********************************��ƹ��� ***********************************
1��֧�ֵ��������ͣ�
	integer : ���ͣ��з��� 4 �ֽ�������
	text : �䳤�ַ�������ʵ�ֵ�ʱ���ڲ�����ֱ����char*��
2��֧���﷨�� 
	SQL�ؼ���(������������)�����ִ�Сд������/����/��ֵ ���ִ�Сд��
    SQL�ؼ��ְ�����
		CREATE TABLE/INTEGER/TEXT/DROP TABLE/INSERT INTO/VALUES/ORDER BY/
		SELECT FROM WHERE/ORDER BY/DELETE FROM WHERE
		SQL����е������ַ�������( ) ' ; = 
3���﷨ϸ�ڣ�
	������䷵�صĴ�����Ϣ����ο�.h�ļ�����
	������CREATE TABLE table_name (col1_name   integer,col2_name   text);�������Ϊ5,�кŴ�1��ʼ
	ɾ����DROP TABLE table_name;
	�����¼��INSERT INTO table_name VALUES (100, 'Bananas');�������ظ�
	��ѯ��¼��SELECT * FROM table_name WHERE col1_name = 100;
			  SELECT * FROM table_name WHERE col2_name = 'Bananas' ORDER BY col1_name;
				1��ORDER BY ����������
				2����ǰSELECT�ؼ��ֺ󣬽�֧��*�������������У���˳���մ�����ʱ�����˳�򣩣�����Ҫ���Ƿ��ؾֲ��е����
*********************************************************************************/
RecordSet select_execute(const char *sql);
RecordSet create_execute(const char *sql);
RecordSet drop_execute(const char *sql);
RecordSet insert_execute(const char *sql);
RecordSet delete_execute(const char *sql);
bool str_compare_ignore_case(const char* var1,const char* var2,int num);
bool str_compare(const char* var1,const char* var2,int num);
int sub_str(const char* var1,char* constraint,int num);
void free_table(struct Table* in);
void free_element(struct element* in);
void free_column(struct column* in);

//////////////////////////////// ��¼���ӿڶ��� ///////////////////////////////

/*****************************************
*	����:	SQL���ִ�нӿ�
*    
*	����:	const char *sql ��  SQL���
*    
*	���:   �����
*     
*	����:	SQL���ִ�к�Ľ������ֻ��SELECT��䷵�صĽ�������ܴ��ڶ���1�������
*			CREATE TABLE/DROP TABLE/INSERT INTO ��䷵�صĽ����ֻ��һ����¼��
*			��ӳ����ǰ����ִ�н�� ��
*			��CREATE TABLEִ�гɹ��󷵻�CREATE TABLE SUCCESS��ʧ���򷵻�CREATE TABLE FAILED
*****************************************/
bool str_compare(const char* var1,const char* var2,int num){
	while(num!=0 && *var1!=0 && *var2!=0){
		if(*var1 == *var2){
			var1++;
			var2++;
			num--;
		}
		else
			return false;
	}
	if(num!=0)return false;
	return true;
}
bool str_compare_ignore_case(const char* var1,const char* var2,int num){
	while(num!=0 && *var1!=0 && *var2!=0){
		if(*var1 == *var2 || *var1 == *var2-32 || *var1 == *var2+32){
			var1++;
			var2++;
			num--;
		}
		else
			return false;
	}
	if(num!=0)return false;
	return true;
}
int sub_str(const char* var1,char* constraint,int num){
	int posi = 0;
	int i;
	while(*(var1+posi)!=0){
		for(i=0;i<num;i++){
		    if(*(var1+posi)==constraint[i])return posi;
		}
		posi++;
	}
	return -1;
}
void free_table(struct Table* in){
	struct element* temp = in->index;
	while(temp!=0){
		in->index = temp->next;
		free_element(temp);
		temp = in->index;
	}
	free_column(in->col);
	free(in->name);
	free(in);
}
void free_element(struct element* in){
	free(in->content);
	free(in);
}
void free_column(struct column* in){
	free(in->name);
	free(in);
}
RecordSet select_execute(const char *sql){

	return NULL;
}
RecordSet create_execute(const char *sql){
	RecordSet result;
	int temp;
	struct Table* table = (struct Table*)malloc(sizeof(struct Table));
	table->col=0;
	table->col_size=0;
	table->index=0;
	table->name=0;
	table->next=0;
	result = (db_record_set *)malloc(sizeof(struct db_record_set));
	result->next = NULL;
	memcpy(result->result,"CREATE TABLE FAILED",20);
	if(!str_compare_ignore_case("create",sql,6))return result;
	sql = sql + 6;
	while(*sql == ' ')  //���ų��ո�
	    sql++;
	if(!str_compare_ignore_case("table",sql,5))return result;
	sql = sql + 5;
	while(*sql == ' ')  //���ų��ո�
	    sql++;

	temp = sub_str(sql," (",2);
	if(temp==-1)return result;
	table->name = (char*)malloc(sizeof(char)*(temp+1));
	memcpy(table->name,sql,temp);
	*(table->name+temp) = 0;
	sql += temp;

	while(*sql == ' ')  //���ų��ո�
	    sql++;
	sql++;//���� ����������ַ�
	while(*sql == ' ')  //���ų��ո�
	    sql++;
	table->col = (struct column*)malloc(sizeof(struct column)*5); //����5��column�ṹ��
	table->col_size = 0;  //ʵ�ʵ�����

	//��һ��
	temp = sub_str(sql," ",1);
	if(temp==-1){
	    free_table(table);	
		return result;
	}
	table->col[table->col_size].name = (char*)malloc(sizeof(char)*(temp+1));
	memcpy(table->col[table->col_size].name, sql, temp);
	*(table->col[table->col_size].name + temp) = 0;
	sql += temp;
	while(*sql == ' ')  //���ų��ո�
	    sql++;
	if(*sql == 'i'){
		if(!str_compare_ignore_case("integer",sql,7)){
			free_table(table);
			return result;
		}
		sql += 7;
		table->col[table->col_size].type = 0;
		table->col_size++;
	}
	else{
		if(!str_compare_ignore_case("text",sql,4)){
			free_table(table);
			return result;
		}
		sql += 4;
		table->col[table->col_size].type = 1;
		table->col_size++;
	}
	while(*sql == ' ')  //���ų��ո�
	    sql++;
	while(*sql ==','){
		sql++;
		//��n��
		while(*sql == ' ')  //���ų��ո�
			 sql++;	
		temp = sub_str(sql," ",1);
		if(temp==-1){
			free_table(table);	
			return result;
		}
		table->col[table->col_size].name = (char*)malloc(sizeof(char)*(temp+1));
		memcpy(table->col[table->col_size].name, sql, temp);
		*(table->col[table->col_size].name + temp) = 0;
		sql += temp;

		while(*sql == ' ')  //���ų��ո�
		  sql++;
		if(*sql == 'i'){
			if(!str_compare_ignore_case("integer",sql,7)){
				free_table(table);
				return result;
			}
			sql += 7;
			table->col[table->col_size].type = 0;
			table->col_size++;
		}
		else{
			if(!str_compare_ignore_case("text",sql,4)){
				free_table(table);
				return result;
			}
			sql += 4;
			table->col[table->col_size].type = 1;
			table->col_size++;
		}
		while(*sql == ' ')  //���ų��ո�
			 sql++;	
	}
	if(*sql != ')'){
		free_table(table);
		return result;
	}
	memcpy(result->result,"CREATE TABLE SUCCESS",21);
	table->next = list;
	list = table;
	return result;
}
RecordSet drop_execute(const char *sql){
	RecordSet result;
	int temp;
	struct Table* table ;
	struct Table* previous ;
	result = (db_record_set *)malloc(sizeof(struct db_record_set));
	result->next = NULL;
	memcpy(result->result,"DROP TABLE FAILED",18);
	if(!str_compare_ignore_case("drop",sql,4))return result;
	sql = sql + 4;
	while(*sql == ' ')  //���ų��ո�
	    sql++;
	if(!str_compare_ignore_case("table",sql,5))return result;
	sql = sql + 5;
	while(*sql == ' ')  //���ų��ո�
	    sql++;
	temp=0;
	while(*(sql+temp)!=' ' && *(sql+temp)!=0 && *(sql+temp)!=';')temp++;
	if(temp==0)return result;

	table = list;
	previous = 0;
	while(table!=0){
		if(str_compare(table->name,sql,temp) && *(table->name+temp)==0){
			if(previous == 0) list = table->next;
			else  previous->next = table->next;
			free_table(table);
			memcpy(result->result,"DROP TABLE SUCCESS",19);
			return result;
		}
		previous = table;
		table = table->next;
	}

	return result;
}
RecordSet insert_execute(const char *sql){
	char cache[20];
	RecordSet result;
	int temp;
	int i;
	int sscanf_num;
	struct Table* table ;
	struct element* element;
	result = (db_record_set *)malloc(sizeof(struct db_record_set));
	result->next = NULL;
	memcpy(result->result,"INSERT INTO FAILED",19);
	if(!str_compare_ignore_case("insert",sql,6))return result;
	sql = sql + 6;
	while(*sql == ' ')  //���ų��ո�
	    sql++;
	if(!str_compare_ignore_case("into",sql,4))return result;
	sql = sql + 4;
	while(*sql == ' ')  //���ų��ո�
	    sql++;
	
    temp = sub_str(sql," ",1);
	if(temp==-1)return result;
	table = list;
	while(table!=0){
		if(str_compare(table->name,sql,temp) && *(table->name+temp)==0){
			break;
		}
		table = table->next;
	}
	if(table == 0) return result;
	sql += temp;
	while(*sql == ' ')  //���ų��ո�
	    sql++;
	if(!str_compare_ignore_case("values",sql,6))return result;
	sql = sql + 6;
	while(*sql == ' ')  //���ų��ո�
	    sql++;
	if(*sql != '(')return result;
	sql++; //���� ����������ַ�

	element = (struct element*)malloc(sizeof(struct element));
	element->next = 0;
	element->content = (int*)malloc(sizeof(int)*table->col_size);
	for(i=0;i<table->col_size-1;i++){
		while(*sql == ' ')  //���ų��ո�
			sql++;
		temp = sub_str(sql," ,",2);
		if(temp==-1)return result;
		if(table->col[i].type==0){
			memcpy(cache,sql,temp);
			*(cache + temp + 1) = 0;
			sscanf(cache,"%d",&(element->content[i]));
			sql += temp;
		}
		else{
			element->content = 0;
			sql += temp;
		}
		while(*sql == ' ')  //���ų��ո�
			sql++;
		if(*sql != ','){
				free_element(element);
				return result;
		}
		sql++; //��������
	}
	while(*sql == ' ')  //���ų��ո�
		sql++;
	temp = sub_str(sql," )",2);
	if(temp==-1)return result;
	if(table->col[i].type==0){
		memcpy(cache,sql,temp);
		*(cache + temp + 1) = 0;
		sscanf_num = sscanf(cache,"%d",&(element->content[i]));
		if(sscanf_num!=1){
			free_element(element);
			return result;
		}
		sql += temp;
	}
	else{
		element->content[i] = (int)malloc(sizeof(char)*temp);
		if(*sql !='\'' || *(sql+temp-1) != '\''){
			free_element(element);
			return result;
		}
		memcpy((char*)(element->content[i]),sql+1,temp-2);
		*((char*)(element->content[i])+temp-1) = 0;
		sql += temp;
	}
	while(*sql == ' ')  //���ų��ո�
		sql++;
	if(*sql != ')'){
		free_element(element);
		return result;
	}
	while(*sql == ' ')  //���ų��ո�
		sql++;
	if(*sql != ';'){
		free_element(element);
		return result;
	}
	memcpy(result->result,"INSERT INTO SUCCESS",20);
	element->next = table->index;
	table->index = element;
	return result;
}
RecordSet delete_execute(const char *sql){
	char cache[20];
	char* column_content1 = 0;
	int column_content0;
	RecordSet result;
	int temp;
	int i;
	int sscanf_num;
	struct Table* table ;
	struct element* element;
	struct element* previous;
	result = (db_record_set *)malloc(sizeof(struct db_record_set));
	result->next = NULL;
	memcpy(result->result,"DELETE FROM FAILED",19);
	if(!str_compare_ignore_case("delete",sql,6))return result;
	sql = sql + 6;
	while(*sql == ' ')  //���ų��ո�
	    sql++;
	if(!str_compare_ignore_case("from",sql,4))return result;
	sql = sql + 4;
	while(*sql == ' ')  //���ų��ո�
	    sql++;
	
    temp = sub_str(sql," ",1);
	if(temp==-1)return result;
	table = list;
	while(table!=0){
		if(str_compare(table->name,sql,temp) && *(table->name+temp)==0){
			break;
		}
		table = table->next;
	}
	if(table == 0) return result;
	sql += temp;
	while(*sql == ' ')  //���ų��ո�
	    sql++;
	if(!str_compare_ignore_case("where",sql,5))return result;
	sql = sql + 5;
	while(*sql == ' ')  //���ų��ո�
	    sql++;

	temp = sub_str(sql," =",2);
	if(temp==-1)return result;
	for(i=0;i<table->col_size;i++){
		if(str_compare(table->col[i].name,sql,temp)){
			break;
		}
	}
	if( i == table->col_size) return result;
	sql += temp;
	while(*sql == ' ')  //���ų��ո�
	    sql++;
	if( *sql != '=') return result;
	sql++;
	while(*sql == ' ')  //���ų��ո�
	    sql++;

	temp=0;
	while(*(sql+temp)!=' ' && *(sql+temp)!=';' && *(sql+temp)!=0)temp++;
	if(temp==0)return result;
	if(table->col[i].type==0){
		    element = table->index;
			previous = 0;
			memcpy(cache,sql,temp);
			*(cache + temp + 1) = 0;
			sscanf_num = sscanf(cache,"%d",&column_content0);
			if(sscanf_num != 1)return result;
			sql += temp;
			while(element != 0){
				if(element->content[i] == column_content0){
					if(previous == 0){
						table->index = table->index->next;
						free_element(element);
						element = table->index; 
					}
					else{
					    previous->next = element->next;
						free_element(element);
						element = previous->next; 
					}
				}
				else{
					previous = element;
					element = element->next;
				}
			}
	}
	else{
		    element = table->index;
			previous = 0;

			column_content1 =  (char*)malloc(sizeof(char)*(temp+1));
			memcpy(column_content1,sql,temp);
			*(column_content1+temp) = 0;
			
			sql += temp;
			while(element != 0){
				if(str_compare( (char*)(element->content[i]),column_content1,temp)){
					if(previous == 0){
						table->index = table->index->next;
						free_element(element);
						element = table->index; 
					}
					else{
					    previous->next = element->next;
						free_element(element);
						element = previous->next; 
					}
				}
				else{
					previous = element;
					element = element->next;
				}
			}
	}

	while(*sql == ' ')  //���ų��ո�
	    sql++;
	if(*sql != ';'){
		if(column_content1 != 0)free(column_content1);
		return result;
	}
	if(column_content1 != 0)free(column_content1);
	memcpy(result->result,"DELETE FROM SUCCESS",20);
	return result;
}

RecordSet sql_execute(const char *sql)
{
	RecordSet result;
	while(*sql == ' ')  //���ų��ո�
	    sql++;

	switch(*sql){
	case's':
		return select_execute(sql);
	case'c':
		return create_execute(sql);
	case'i':
		return insert_execute(sql);
	case'd':
		if(*(sql+1)== 'r')return drop_execute(sql);
        return delete_execute(sql);
	default:
        result = (db_record_set *)malloc(sizeof(struct db_record_set));
		result->next = NULL;
		memcpy(result->result,"syntax error",13);
		return result;
	}
}


/*****************************************
*	����:	SQL���ִ�нӿ�
			������ sql_execute �ӿ��Ѿ�˵���������ֻ�������������
*			(1) CREATE TABLE/DROP TABLE/INSERT INTO ��䷵�صĽ����ֻ��һ����¼��
*			ʹ�÷�ʽ���£�
*			RecordSet rs = 
*			sql_execute("CREATE TABLE table_select_multi ( id integer, name text );");
*			Record rcd = get_record(rs, 0);
*			char * rcd_s = record_to_string(rcd);
*			int ret = strcmp(rcd_s, "CREATE TABLE SUCCESS");
*
*			(2) SELECT��䷵�صĽ�������ܴ��ڶ���1���������ʹ�÷�ʽ��
*			RecordSet rs = sql_execute("SELECT * FROM table_select_multi ORDER BY id;");
*			// ȡ��һ��
*			Record rcd = get_record(rs, 0);
*			char * rcd_s = record_to_string(rcd);
*			ret = strcmp(rcd_s, "100, 'Bananas'");
*			CPPUNIT_ASSERT(ret == 0);
*			// ȡ�ڶ���
*			rcd = get_record(rs, 1);
*			rcd_s = record_to_string(rcd);
*			ret = strcmp(rcd_s, "200, 'Apple'");
*			CPPUNIT_ASSERT(ret == 0);
*    
*	����:	const RecordSet rs ��  �����
*			int index ��           �кţ��кŴ�0��ʼ��
*    
*	���:   �����
*     
*	����:	�����rs�еĵ�index����¼
*****************************************/

Record get_record(const RecordSet rs, int index)
{
	return NULL;
}

/******************************************************************************
 * �������� :	  �ͷż�¼���е�������Դ����̬������ڴ棩
 *                �ر�˵������¼���е��ڴ�����������漸������·���ģ�
 *                1����¼���е��ڴ��������� sql_execute �����з���ģ�
 *                2���ڵ��� get_record ��ʱ���п��ܻ�����ڴ棨��ʵ�ַ�ʽ�йأ�
 *                3���ڵ��� record_to_string ��ʱ�������ڴ棬ҲҪ��������
 *
 * ����:		  [IN] ��¼��
 *
 * ���:		  �����
 *
 * ����:		  ���ô˽ӿں�rs �е��ڴ�ᱻ�ͷ���ϣ����rs��ֵΪNULL
******************************************************************************/

void rm_recordset(RecordSet& rs)
{
	RecordSet next;
	while(rs != 0){
		next = rs->next;
		free(rs);
		rs = next;
	}
	return;
}
/******************************************************************************
 * �������� :	  ����¼ת�����ַ����������Ϳ��Ա���У����
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
 * ����  :		  [IN] ��¼
 *
 * ���  :		  �������
 *
 * ����  :		  ��¼ת�ɵ��ַ���
 *				  �ر�˵�������ص��ַ������ڴ���Ȼ��SQL��������ں���rm_recordset
 *				  ��һͬ�ͷ�
******************************************************************************/
char* record_to_string(const Record rcd)
{
	return NULL;
}


