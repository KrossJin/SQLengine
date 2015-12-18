//krossjin
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
/***********************************设计规则 ***********************************
1、支持的数据类型：
	integer : 整型，有符号 4 字节整数；
	text : 变长字符串；（实现的时候，内部可以直接用char*）
2、支持语法： 
	SQL关键字(包括数据类型)不区分大小写，表名/列名/列值 区分大小写；
    SQL关键字包括：
		CREATE TABLE/INTEGER/TEXT/DROP TABLE/INSERT INTO/VALUES/ORDER BY/
		SELECT FROM WHERE/ORDER BY/DELETE FROM WHERE
		SQL语句中的特殊字符包括：( ) ' ; = 
3、语法细节：
	关于语句返回的错误信息，请参考.h文件定义
	创建表：CREATE TABLE table_name (col1_name   integer,col2_name   text);最大列数为5,列号从1开始
	删除表：DROP TABLE table_name;
	插入记录：INSERT INTO table_name VALUES (100, 'Bananas');允许有重复
	查询记录：SELECT * FROM table_name WHERE col1_name = 100;
			  SELECT * FROM table_name WHERE col2_name = 'Bananas' ORDER BY col1_name;
				1、ORDER BY 以升序排列
				2、当前SELECT关键字后，仅支持*（即返回所有列，列顺序按照创建表时候的列顺序），不需要考虑返回局部列的情况
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

//////////////////////////////// 记录集接口定义 ///////////////////////////////

/*****************************************
*	功能:	SQL语句执行接口
*    
*	输入:	const char *sql ：  SQL语句
*    
*	输出:   无输出
*     
*	返回:	SQL语句执行后的结果集，只有SELECT语句返回的结果集可能存在多于1条的情况
*			CREATE TABLE/DROP TABLE/INSERT INTO 语句返回的结果集只有一条记录，
*			反映出当前语句的执行结果 。
*			如CREATE TABLE执行成功后返回CREATE TABLE SUCCESS，失败则返回CREATE TABLE FAILED
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
	while(*sql == ' ')  //先排除空格
	    sql++;
	if(!str_compare_ignore_case("table",sql,5))return result;
	sql = sql + 5;
	while(*sql == ' ')  //先排除空格
	    sql++;

	temp = sub_str(sql," (",2);
	if(temp==-1)return result;
	table->name = (char*)malloc(sizeof(char)*(temp+1));
	memcpy(table->name,sql,temp);
	*(table->name+temp) = 0;
	sql += temp;

	while(*sql == ' ')  //先排除空格
	    sql++;
	sql++;//跳过 ‘（’这个字符
	while(*sql == ' ')  //先排除空格
	    sql++;
	table->col = (struct column*)malloc(sizeof(struct column)*5); //申请5个column结构体
	table->col_size = 0;  //实际的列数

	//第一列
	temp = sub_str(sql," ",1);
	if(temp==-1){
	    free_table(table);	
		return result;
	}
	table->col[table->col_size].name = (char*)malloc(sizeof(char)*(temp+1));
	memcpy(table->col[table->col_size].name, sql, temp);
	*(table->col[table->col_size].name + temp) = 0;
	sql += temp;
	while(*sql == ' ')  //先排除空格
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
	while(*sql == ' ')  //先排除空格
	    sql++;
	while(*sql ==','){
		sql++;
		//第n列
		while(*sql == ' ')  //先排除空格
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

		while(*sql == ' ')  //先排除空格
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
		while(*sql == ' ')  //先排除空格
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
	while(*sql == ' ')  //先排除空格
	    sql++;
	if(!str_compare_ignore_case("table",sql,5))return result;
	sql = sql + 5;
	while(*sql == ' ')  //先排除空格
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
	while(*sql == ' ')  //先排除空格
	    sql++;
	if(!str_compare_ignore_case("into",sql,4))return result;
	sql = sql + 4;
	while(*sql == ' ')  //先排除空格
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
	while(*sql == ' ')  //先排除空格
	    sql++;
	if(!str_compare_ignore_case("values",sql,6))return result;
	sql = sql + 6;
	while(*sql == ' ')  //先排除空格
	    sql++;
	if(*sql != '(')return result;
	sql++; //跳过 ‘（’这个字符

	element = (struct element*)malloc(sizeof(struct element));
	element->next = 0;
	element->content = (int*)malloc(sizeof(int)*table->col_size);
	for(i=0;i<table->col_size-1;i++){
		while(*sql == ' ')  //先排除空格
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
		while(*sql == ' ')  //先排除空格
			sql++;
		if(*sql != ','){
				free_element(element);
				return result;
		}
		sql++; //跳过，号
	}
	while(*sql == ' ')  //先排除空格
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
	while(*sql == ' ')  //先排除空格
		sql++;
	if(*sql != ')'){
		free_element(element);
		return result;
	}
	while(*sql == ' ')  //先排除空格
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
	while(*sql == ' ')  //先排除空格
	    sql++;
	if(!str_compare_ignore_case("from",sql,4))return result;
	sql = sql + 4;
	while(*sql == ' ')  //先排除空格
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
	while(*sql == ' ')  //先排除空格
	    sql++;
	if(!str_compare_ignore_case("where",sql,5))return result;
	sql = sql + 5;
	while(*sql == ' ')  //先排除空格
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
	while(*sql == ' ')  //先排除空格
	    sql++;
	if( *sql != '=') return result;
	sql++;
	while(*sql == ' ')  //先排除空格
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

	while(*sql == ' ')  //先排除空格
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
	while(*sql == ' ')  //先排除空格
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
*	功能:	SQL语句执行接口
			从上面 sql_execute 接口已经说明，结果集只有两种类情况：
*			(1) CREATE TABLE/DROP TABLE/INSERT INTO 语句返回的结果集只有一条记录，
*			使用方式如下：
*			RecordSet rs = 
*			sql_execute("CREATE TABLE table_select_multi ( id integer, name text );");
*			Record rcd = get_record(rs, 0);
*			char * rcd_s = record_to_string(rcd);
*			int ret = strcmp(rcd_s, "CREATE TABLE SUCCESS");
*
*			(2) SELECT语句返回的结果集可能存在多于1条的情况，使用方式：
*			RecordSet rs = sql_execute("SELECT * FROM table_select_multi ORDER BY id;");
*			// 取第一行
*			Record rcd = get_record(rs, 0);
*			char * rcd_s = record_to_string(rcd);
*			ret = strcmp(rcd_s, "100, 'Bananas'");
*			CPPUNIT_ASSERT(ret == 0);
*			// 取第二行
*			rcd = get_record(rs, 1);
*			rcd_s = record_to_string(rcd);
*			ret = strcmp(rcd_s, "200, 'Apple'");
*			CPPUNIT_ASSERT(ret == 0);
*    
*	输入:	const RecordSet rs ：  结果集
*			int index ：           行号（行号从0开始）
*    
*	输出:   无输出
*     
*	返回:	结果集rs中的第index条记录
*****************************************/

Record get_record(const RecordSet rs, int index)
{
	return NULL;
}

/******************************************************************************
 * 功能描述 :	  释放记录集中的所有资源（动态分配的内存）
 *                特别说明，记录集中的内存可能是在下面几种情况下分配的：
 *                1、记录集中的内存主体是在 sql_execute 函数中分配的；
 *                2、在调用 get_record 的时候有可能会分配内存（跟实现方式有关）
 *                3、在调用 record_to_string 的时候会分配内存，也要管理起来
 *
 * 输入:		  [IN] 记录集
 *
 * 输出:		  无输出
 *
 * 返回:		  调用此接口后，rs 中的内存会被释放完毕，最后rs的值为NULL
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
 * 功能描述 :	  将记录转换成字符串，这样就可以便于校验结果
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
 * 输入  :		  [IN] 记录
 *
 * 输出  :		  无输出。
 *
 * 返回  :		  记录转成的字符串
 *				  特别说明：返回的字符串的内存仍然由SQL引擎管理，在函数rm_recordset
 *				  中一同释放
******************************************************************************/
char* record_to_string(const Record rcd)
{
	return NULL;
}


