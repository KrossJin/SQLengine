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
bool table_exist(const char* name);
bool smaller_than(struct element* var1,struct element* var2,int i,int type);
int sub_str(const char* var1,char* constraint,int num);
int str_len(const char* var);
void free_table(struct Table* in);
void free_element(struct element* in);
void free_column(struct column* in);
void sort(struct element** in,int i);
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
bool table_exist(const char* name){
	struct Table* temp = list;
	while(temp != 0){
		if(strcmp(temp->name,name) == 0)return true;
		temp = temp->next;
	}
	return false;
}
bool smaller_than(struct element* var1,struct element* var2,int i,int type){ //由于在生成 final_re的时候有一个顺序倒转，所以我们生成的out要是 从大到小排列才行
	char* v1;
	char* v2;
	if(type==0){
		return (var1->content[i] > var2->content[i]);
	}
	v1 = (char*)(var1->content[i]);
	v2 = (char*)(var2->content[i]);
	while(*v1 != 0 && *v2 != 0){
		if(*v1 != *v2){
			return (*v1 > *v2);
		}
		v1++;
		v2++;
	}
	if(*v1 != 0)return true;
	return false;
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
int str_len(const char* var){
	int len  = 0;
	while(*(var+len) != 0)len++;
	return len;
}
void free_table(struct Table* in){
	struct element* temp;
	if(in == 0)return;
	temp= in->index;
	while(temp!=0){
		in->index = temp->next;
		free_element(temp);
		temp = in->index;
	}
	
	free_column(in->col);
	if(in->name != 0) free(in->name);
	free(in);
}
void free_element(struct element* in){
	if(in == 0)return ;
	if(in->content != 0)free(in->content);
	free(in);
}
void free_column(struct column* in){
	if(in == 0)return;
	if(in->name != 0)free(in->name);
	free(in);
}
void sort(struct element** in,int i,int type){
	struct element* src  = *in;
	struct element* dest = 0;
	struct element* temp;
	struct element* previous;
	if(src!=0){
		dest = src;
		src = src->next;
		dest->next = 0;
		while(src!=0){
			temp = dest;
			previous = 0;
			while(temp !=0 && !smaller_than(src,temp,i,type)){
			    previous = temp;
				temp = temp->next;
			}
			if(temp == 0){
				previous->next = src;
				src = src->next;
				previous->next->next = 0;
			}
			else{
				if(previous == 0){
				    dest = src;
					src = src->next;
					dest->next = temp;
				}
				else{
				    previous->next = src;
					src = src->next;
					previous->next->next = temp;
				}
			}
		}
		*in = dest;
	}
}
RecordSet select_execute(const char *sql){
	char cache[1024];                           //临时缓存区，主要放一些字符串
	char* column_content1 = 0;                //如果where后面的数据是字符串，就将字符串存放在这儿
	int column_content0;                      //如果where后面的数据是数字，就将数字存放在这儿
	struct element* out = 0;                  //用来存储挑选得到的结果
	struct element* added = 0;                //临时 malloc element时，使用
	RecordSet final_re = 0;                   //存储最终的结果
	int temp;
	int i;
	int sscanf_num;
	struct Table* table ;                     
	struct element* element;                  //删除的时候，用作current指针
/*******返回结果为失败的初始化过程**********************************************/
	RecordSet result;
	result = (db_record_set *)malloc(sizeof(struct db_record_set));
	result->next = NULL;
	memcpy(result->result,"SELECT FROM FAILED",19);
/*******返回结果为失败的初始化过程**********************************************/

	if(!str_compare_ignore_case("select",sql,6))return result;
	sql = sql + 6;
	if(*sql != ' ')return result;                         //判断 第一个关键字和 第二个关键字之间是否有空格，没有就报错。
	while(*sql == ' ') sql++;                             //清除多余的空格
	if(*sql != '*')return result;                         //判断 第一个关键字 后面必须是 * ，没有就报错。
	sql++;                                                //跳过 * 号
	if(*sql != ' ')return result;                         //判断 * 号和 第二个关键字之间是否有空格，没有就报错。
	while(*sql == ' ') sql++;                             //清除多余的空格
	if(!str_compare_ignore_case("from",sql,4))return result;
	sql = sql + 4;
	if(*sql != ' ')return result;                         //判断 第二个关键字和 table_name之间是否有空格，没有就报错。
	while(*sql == ' ') sql++;                             //清除多余的空格
/********************获取 table_name **************************/
    temp = sub_str(sql," ",1);                           //table_name只能以 空格 结尾
	if(temp==-1)return result;
	
	/**********判断table_name为名字的数据表是否存在*************/
	table = list;
	while(table!=0){
		if(str_compare(table->name,sql,temp) && *(table->name+temp)==0){
			break;
		}
		table = table->next;
	}
	if(table == 0) return result;                            //不存在
	sql += temp;                                             //这句话只能放在这儿，因为上面的while循环利用sql时，sql要指向数据表名字的头

	while(*sql == ' ') sql++;                                //清除多余的空格
	if(!str_compare_ignore_case("where",sql,5))return result;
	sql = sql + 5;
	if(*sql != ' ') return result;                           //where后面 必须有一个 空格
	while(*sql == ' ') sql++;                                //清除多余的空格
/********************获取 where条件判断时，列的名字 **************************/
	temp = sub_str(sql," =",2);                             //列的名字后面跟着空格或者等于号
	if(temp==-1)return result;
	for(i=0;i<table->col_size;i++){
		if(str_compare(table->col[i].name,sql,temp) && *(table->col[i].name+temp)==0){
			break;
		}
	}
	if( i == table->col_size) return result;               //列不存在，返回错误
	sql += temp;
	while(*sql == ' ') sql++;                              //清除多余的空格
	if( *sql != '=') return result;                        //列名后面必须有 等于号
	sql++;                                                 //跳过等于号
	while(*sql == ' ') sql++;                              //清除多余的空格
/********************获取列的值 **************************/
	temp=0;
	while(*(sql+temp)!=' ' && *(sql+temp)!=';' && *(sql+temp)!=0)temp++;
	if(temp==0)return result;
	if(table->col[i].type==0){                             //由于 i 是从上面for循环中跳出来的，所以 i 代表列名所指向的那个列
		    element = table->index;
			/**************将字符串转换为数字***********/
			memcpy(cache,sql,temp);                       
			*(cache + temp) = 0;
			sscanf_num = sscanf(cache,"%d",&column_content0);
			if(sscanf_num != 1)return result;
			sql += temp;
			/**************将字符串转换为数字***********/
			while(element != 0){
				if(element->content[i] == column_content0){  //在table里面有一个满足条件的element，就添加到out结果集里面一个，所以if满足条件，也不会跳出
					added = (struct element*)malloc(sizeof(struct element));
					added->content = element->content;
					added->next = out;
					out = added;
				}
				element = element->next;	
			}
	}
	else{                                    //另一种类型，text
		    element = table->index;
			if(*sql != '\'' ||*(sql+temp-1) != '\'' ) return result;
			/*******************字符串处理************************/
			column_content1 =  (char*)malloc(sizeof(char)*(temp-1));			
			memcpy(column_content1,sql+1,temp-2);                        //把字符串临时存到column_content1里面，第一个单引号，和最后一个单引号去掉
			*(column_content1+temp-2) = 0;			
			sql += temp;
			/*******************字符串处理************************/

			while(element != 0){
				if(str_compare( (char*)(element->content[i]),column_content1,temp-2)){//在table里面有一个满足条件的element，就添加到out结果集里面一个，所以if满足条件，也不会跳出
					added = (struct element*)malloc(sizeof(struct element));
					added->content = element->content;
					added->next = out;
					out = added;
				}
				element = element->next;
				
			}
	}
	
	while(*sql == ' ') sql++;                             //清除多余的空格,就算 order紧跟在值的后面，在其他地方也一样会报错，所以这里，我们就直接清除空格了

	if(*sql == ';'){                                     //没有order by的语句处理方式
		if(column_content1 != 0)free(column_content1);  
		free(result);                                   //为了在while循环中统一格式，我们只能先free掉这个 result
		added = out;
		while(added != 0){
			temp = 0;
			for(i=0;i<table->col_size;i++){
				if(table->col[i].type==0){ 				
				    sprintf((cache+temp),"%d, ",added->content[i]);
					while(cache[temp] != ' ')temp++;
					temp++;                     //跳过空格
					if(i == table->col_size-1) temp -= 2; //最后一列的格式比较特殊	
				}
				else{
					sprintf((cache+temp),"'%s', ",(char*)(added->content[i]));
					temp = temp + str_len((char*)(added->content[i])) + 4; //while(cache[temp] != ' ')temp++; 不使用这个语句的原因是担心 字符串中有自带的空格
					if(i == table->col_size-1) temp -= 2; //最后一列的格式比较特殊	
				}		
			}
			cache[temp] = 0;                                           //为记录添加 字符串结束符
			result = (RecordSet)malloc(sizeof(struct db_record_set));
			memcpy(result->result,cache,temp+1);                       //将一个记录的结果拷贝到 最终结果集里面去
			result->next = final_re;
			final_re = result;
			
			added = added->next;
		}
		while(out !=0 ){                                  //释放掉临时的结果集
			added = out->next;
			free(out);
			out = added;
		}
		return final_re;
	}
/***************************有order by的处理方式**************************************/
	if(column_content1 != 0)free(column_content1);        //别忘了释放 临时缓存字符串的指针
/*********************继续 探测 order by的语法************************/
	if(!str_compare_ignore_case("order",sql,5))return result;
	sql = sql + 5;
	if(*sql != ' ')return result;                         //判断 order 和 by  之间是否有空格，没有就报错。
	while(*sql == ' ') sql++;                             //清除多余的空格
	if(!str_compare_ignore_case("by",sql,2))return result;
	sql = sql + 2;
	if(*sql != ' ')return result;                         //判断 第二个关键字和 column_name之间是否有空格，没有就报错。
	while(*sql == ' ') sql++;                             //清除多余的空格
/********************获取 column_name **************************/
    temp=0;
	while(*(sql+temp)!=' ' && *(sql+temp)!=';' && *(sql+temp)!=0)temp++;   //column_name 后面只能跟着 空格 ; 还有 \0
	if(temp==0){                                          //order by后面没有东西了，报错
		while(out !=0 ){                                  //释放掉临时的结果集
			added = out->next;
			free(out);
			out = added;
		}
		return result;
	}                            
	/**********  for循环的作用就是找到column_name对应的下标*********************/
	for(i=0;i<table->col_size;i++){
		if(str_compare(table->col[i].name,sql,temp) && *(table->col[i].name+temp)==0){
			break;
		}
	}
	sort(&out,i,table->col[i].type);

	free(result);                                   //为了在while循环中统一格式，我们只能先free掉这个 result
	added = out;
	while(added != 0){
		temp = 0;
		for(i=0;i<table->col_size;i++){
			if(table->col[i].type==0){ 				
			    sprintf((cache+temp),"%d, ",added->content[i]);
				while(cache[temp] != ' ')temp++;
				temp++;                     //跳过空格
				if(i == table->col_size-1) temp -= 2; //最后一列的格式比较特殊	
			}
			else{
				sprintf((cache+temp),"'%s', ",(char*)(added->content[i]));
				temp = temp + str_len((char*)(added->content[i])) + 4; //while(cache[temp] != ' ')temp++; 不使用这个语句的原因是担心 字符串中有自带的空格
				if(i == table->col_size-1) temp -= 2; //最后一列的格式比较特殊	
			}		
		}
		cache[temp] = 0;                                           //为记录添加 字符串结束符
		result = (RecordSet)malloc(sizeof(struct db_record_set));
		memcpy(result->result,cache,temp+1);                       //将一个记录的结果拷贝到 最终结果集里面去
		result->next = final_re;
		final_re = result;
		
		added = added->next;
	}
	while(out !=0 ){                                  //释放掉临时的结果集
		added = out->next;
		free(out);
		out = added;
	}
	return final_re;
}
RecordSet create_execute(const char *sql){
	RecordSet result;        //用来返回结果
	int temp;                //存储 sub_str（）返回的结束符的位置

/**********在内存中的Table数据结构初始化，如果创建失败，不要忘记 free 掉**********************************/
	struct Table* table = (struct Table*)malloc(sizeof(struct Table)); 
	table->col=0;    
	table->col_size=0;
	table->index=0;
	table->name=0;
	table->next=0;
/**********在内存中的Table数据结构初始化，如果创建失败，不要忘记 free 掉***************************/


/*******返回结果为失败的初始化过程**********************************************/
	result = (db_record_set *)malloc(sizeof(struct db_record_set));
	result->next = NULL;
	memcpy(result->result,"CREATE TABLE FAILED",20);
/*******返回结果为失败的初始化过程*********************************************/

/**************传入的字符串前端的空格已被消除掉************************************/
/******************前两个关键字的处理************************************/
	if(!str_compare_ignore_case("create",sql,6)){free(table);  return result;}   //判断关键字是否正确。此外，由于table的元素尚未初始化，
	                                                                             //所以释放table的时候，直接用free函数就可以
	sql = sql + 6;                                         //字符串指针前移
	if(*sql != ' ') {free(table);  return result;}         //判断 第一个关键字和 第二个关键字之间是否有空格，没有就报错。
	
	while(*sql == ' ') sql++;                             //清除多余的空格
	    
	if(!str_compare_ignore_case("table",sql,5)){free(table);  return result;}   //判断关键字是否正确。此外，由于table的元素尚未初始化，
	                                                                             //所以释放table的时候，直接用free函数就可以
	sql = sql + 5;
	if(*sql != ' ') {free(table);  return result;}         //判断 第二个关键字和 table_name 之间是否有空格，没有就报错。
	while(*sql == ' ')  sql++;                            //清除多余的空格
/******************table_name 的处理************************************/	   
	temp = sub_str(sql," (",2);                                         //把 table_name 提取出来
	if(temp==-1){free(table);  return result;}                          //返回值是 -1，提取失败
	table->name = (char*)malloc(sizeof(char)*(temp+1));                 //temp多加1的目的就是最后一个字节存放结束符 \0
	memcpy(table->name,sql,temp);
	*(table->name+temp) = 0;                             
	sql += temp;
	if(table_exist(table->name)){ free_table(table);  return result;}  //这个表已经存在了，错误返回
/******************（）里面内容的处理************************************/
	while(*sql == ' ')    sql++;                             //清除多余的空格
	if(*sql != '(') { free_table(table);  return result;}    //判断 左括号是否存在 ，没有就报错。
	sql++;                                                   //跳过 左括号 这个字符
	while(*sql == ' ')    sql++;                             //清除多余的空格

	table->col = (struct column*)malloc(sizeof(struct column)*5); //申请5个column结构体，因为column最多就是5个，所以用不到的话先空着
	table->col_size = 0;                                          //实际的列数，标志实际使用个数

	/*******************第一个column的处理***********************/
	temp = sub_str(sql," ",1);
	if(temp==-1){ free_table(table);   return result;}           //返回值是 -1，提取失败
	/*******column的name获取******/
	table->col[table->col_size].name = (char*)malloc(sizeof(char)*(temp+1));
	memcpy(table->col[table->col_size].name, sql, temp);
	*(table->col[table->col_size].name + temp) = 0;
	sql += temp;
	while(*sql == ' ')    sql++;                             //清除多余的空格

	/*******column的type获取******/
	if(*sql == 'i'){
		if(!str_compare_ignore_case("integer",sql,7)){
			free_table(table); 
			temp = sub_str(sql," ,",2);
			memcpy(result->result,"Type:'",6);
			memcpy(result->result+6+temp,"' is not supported",19);
			temp--;
			while(temp >= 0){
			   *(result->result + 6 + temp) = *(sql + temp); 
			}
			return result;
		} //类型不对，错误返回
		sql += 7;
		table->col[table->col_size].type = 0;
		table->col_size++;
	}
	else{
		if(!str_compare_ignore_case("text",sql,4)) { 
				free_table(table); 
				temp = sub_str(sql," ,",2);
				memcpy(result->result,"Type:'",6);
				memcpy(result->result+6+temp,"' is not supported",19);
				temp--;
				while(temp >= 0){
				  *(result->result + 6 + temp) = *(sql + temp); 
				  temp--;
				}
				return result;
		}
		sql += 4;
		table->col[table->col_size].type = 1;
		table->col_size++;
	}
	while(*sql == ' ')    sql++;                                           //清除多余的空格

	while(*sql ==','){                                                     //一个column 后面是逗号，说明还有column
		if(table->col_size == 5){ free_table(table); return result;}       //已经获取5个column了，后面还有逗号，报错返回
		sql++;                                                             //跳过 逗号 这个字符
		/***************第n列的处理过程**********************/
		while(*sql == ' ')    sql++;                             //清除多余的空格
			 
		temp = sub_str(sql," ",1);
		if(temp==-1){ free_table(table); return result;}
		table->col[table->col_size].name = (char*)malloc(sizeof(char)*(temp+1));
		memcpy(table->col[table->col_size].name, sql, temp);
		*(table->col[table->col_size].name + temp) = 0;
		sql += temp;

		while(*sql == ' ')    sql++;                             //清除多余的空格
		if(*sql == 'i'){
			if(!str_compare_ignore_case("integer",sql,7)){free_table(table);return result;}
			sql += 7;
			table->col[table->col_size].type = 0;
			table->col_size++;
		}
		else{
			if(!str_compare_ignore_case("text",sql,4)){free_table(table);return result;}
			sql += 4;
			table->col[table->col_size].type = 1;
			table->col_size++;
		}
		while(*sql == ' ')    sql++;                             //清除多余的空格
	}
/****************以上括号内的column都处理完了*************************/

	if(*sql != ')'){free_table(table);return result; }        //一定要有右括号，不然语法错误
	sql++;                                                   //跳过右括号
	while(*sql == ' ')    sql++;                             //清除多余的空格
	if(*sql != ';'){free_table(table);return result; }       //一定要有';'结束符，不然语法错误
	/***************终于解析完了，成功！！！！！！**************/
	table->next = list;                             //插入数据表到链表当中。 
	list = table;

	memcpy(result->result,"CREATE TABLE SUCCESS",21);
	return result;
}
RecordSet drop_execute(const char *sql){
	RecordSet result;        //用来返回结果
	int temp;                //存储 sub_str（）返回的结束符的位置
	struct Table* table ;    //在删除数据表的过程中作为 current 指针
	struct Table* previous ;
/*******返回结果为失败的初始化过程**********************************************/
	result = (db_record_set *)malloc(sizeof(struct db_record_set));
	result->next = NULL;
	memcpy(result->result,"DROP TABLE FAILED",18);
/*******返回结果为失败的初始化过程**********************************************/

	if(!str_compare_ignore_case("drop",sql,4))return result;
	sql = sql + 4;
	if(*sql != ' ') return result;                        //判断 第一个关键字和 第二个关键字之间是否有空格，没有就报错。
	
	while(*sql == ' ') sql++;                             //清除多余的空格
	if(!str_compare_ignore_case("table",sql,5))return result;
	sql = sql + 5;
	if(*sql != ' ')  return result;                      //判断 第二个关键字和 table_name之间是否有空格，没有就报错。
	
	while(*sql == ' ') sql++;                             //清除多余的空格

	temp=0;
	while(*(sql+temp)!=' ' && *(sql+temp)!=0 && *(sql+temp)!=';')temp++;  //计算 table_name的长度
	if(temp==0)return result;                                             //table_name不存在

	table = list;
	previous = 0;
	while(table!=0){                                                      //遍历 数据表的链表，查看要删除的数据表是否存在
		if(str_compare(table->name,sql,temp) && *(table->name+temp)==0){  //如果存在
			sql+=temp;
			while(*sql == ' ') sql++;                                             //清除多余的空格
		    if(*sql != ';')return result;                                         // ';'不存在,返回错误信息。
			if(previous == 0) list = table->next;
			else  previous->next = table->next;
			free_table(table);
			memcpy(result->result,"DROP TABLE SUCCESS",19);
			return result;
		}
		previous = table;
		table = table->next;
	}
	//如果不存在，返回 错误信息
	return result;
}
RecordSet insert_execute(const char *sql){
	char cache[20];                       //临时缓存区，主要放一些字符串
	int temp;
	int i;
	int sscanf_num;
	struct Table* table ;
	struct element* element;             
/*******返回结果为失败的初始化过程**********************************************/
	RecordSet result;
	result = (db_record_set *)malloc(sizeof(struct db_record_set));
	result->next = NULL;
	memcpy(result->result,"INSERT INTO FAILED",19);
/*******返回结果为失败的初始化过程**********************************************/

	if(!str_compare_ignore_case("insert",sql,6))return result;
	sql = sql + 6; 
	if(*sql != ' ') return result;                        //判断 第一个关键字和 第二个关键字之间是否有空格，没有就报错。
	
	while(*sql == ' ') sql++;                             //清除多余的空格
	if(!str_compare_ignore_case("into",sql,4))return result;
	sql = sql + 4;
	if(*sql != ' ') return result;                        //判断 第二个关键字和 table_name 之间是否有空格，没有就报错。
	
	while(*sql == ' ') sql++;                             //清除多余的空格
	
	/***********获取table_name********************/
    temp = sub_str(sql," ",1);                           //table_name后面是values，不是（）
	if(temp==-1) return result;                          //table_name获取失败，返回错误信息

	/***********判断table是否存在************************/
	table = list;
	while(table!=0){
		if(str_compare(table->name,sql,temp) && *(table->name+temp)==0){
			break;
		}
		table = table->next;
	}
	if(table == 0) return result;                                //table不存在
	sql += temp;                                                 //指针的移动必须在table_name比较完以后，再行移动
    /****if(*sql != ' ') return result;                           由于前面table_name结束的标志就是 空格 ，所以此处不必再做判断****/
	while(*sql == ' ') sql++;                                    //清除多余的空格，table_name后面
	if(!str_compare_ignore_case("values",sql,6))return result;  
	sql = sql + 6;
	while(*sql == ' ') sql++;                                    //清除多余的空格
	if(*sql != '(')return result;                                //values后面不是 （，返回错误信息
	sql++;                                                       //跳过 ‘（’这个字符

/******************element结构体初始化，如果insert失败，注意释放 空间资源*****************************/
	element = (struct element*)malloc(sizeof(struct element));
	element->next = 0;
	element->content = (int*)malloc(sizeof(int)*table->col_size);  //element有多少column，根据table的column_size来定
	/********** for循环主要是将值挨个取出 **********/
	for(i=0;i<table->col_size-1;i++){
		while(*sql == ' ') sql++;                             //清除多余的空格
		temp = sub_str(sql," ,",2);                           //一个值得后面要么是 空格，要么是 逗号
		if(temp==-1){ free_element(element); return result; } //如果取值失败
		/**********根据值的具体类型，来判断字符串是转换为数字，还是单纯的字符串********/
		if(table->col[i].type==0){
				memcpy(cache,sql,temp);
				*(cache + temp + 1) = 0;
				sscanf_num = sscanf(cache,"%d",&(element->content[i]));
				if(sscanf_num!=1){ free_element(element); return result; } //数字不纯正，失败
				sql += temp;
		}
		else{
				element->content[i] = (int)malloc(sizeof(char)*temp);
				if(*sql !='\'' || *(sql+temp-1) != '\''){ free_element(element); return result; } //字符串的前后必须都是单引号
				memcpy((char*)(element->content[i]),sql+1,temp-2);
				*((char*)(element->content[i])+temp-2) = 0;
				sql += temp;
		}
		/**********根据值的具体类型，来判断字符串是转换为数字，还是单纯的字符串********/
		while(*sql == ' ') sql++;                                 //清除多余的空格
		if(*sql != ','){ free_element(element); return result; }  //值的后面必有一个 逗号，否则语法错误
		sql++;                                                    //跳过，号
	}
	while(*sql == ' ') sql++;                                     //清除多余的空格
	temp = sub_str(sql," )",2);                                   //最后一个值得后面要么是 空格，要么是 右括号
	if(temp==-1){free_element(element);  return result;}          //取值失败，free掉 element后，返回错误信息
/**********根据值的具体类型，来判断字符串是转换为数字，还是单纯的字符串********/
	if(table->col[i].type==0){
		memcpy(cache,sql,temp);
		*(cache + temp + 1) = 0;
		sscanf_num = sscanf(cache,"%d",&(element->content[i]));
		if(sscanf_num!=1){ free_element(element); return result; } //数字不纯正，失败，      注意：这儿  234sd可能也当作整数
		sql += temp;
	}
	else{
		element->content[i] = (int)malloc(sizeof(char)*temp);
		if(*sql !='\'' || *(sql+temp-1) != '\''){ free_element(element); return result; } //字符串的前后必须都是单引号
		memcpy((char*)(element->content[i]),sql+1,temp-2);
		*((char*)(element->content[i])+temp-2) = 0;
		sql += temp;
	}
/**********根据值的具体类型，来判断字符串是转换为数字，还是单纯的字符串********/
	while(*sql == ' ') sql++;                               //清除多余的空格
	if(*sql != ')'){free_element(element); return result; } //右括号不能少
	sql++;                                                  //跳过右括号
	while(*sql == ' ') sql++;                               //清除多余的空格
	if(*sql != ';'){ free_element(element);return result;}  // ;结束符不能少
	/********成功insert，将element添加到table中去***********/
	element->next = table->index;                           //注意，可以重复添加一个项，所以不做存在验证
	table->index = element;

	memcpy(result->result,"INSERT INTO SUCCESS",20);
	return result;
}
RecordSet delete_execute(const char *sql){
	char cache[20];                           //临时缓存区，主要放一些字符串
	char* column_content1 = 0;                //如果where后面的数据是字符串，就将字符串存放在这儿
	int column_content0;                      //如果where后面的数据是数字，就将数字存放在这儿
	int temp;
	int i;
	int sscanf_num;
	struct Table* table ;                     
	struct element* element;                  //删除的时候，用作current指针
	struct element* previous;
/*******返回结果为失败的初始化过程**********************************************/
	RecordSet result;
	result = (db_record_set *)malloc(sizeof(struct db_record_set));
	result->next = NULL;
	memcpy(result->result,"DELETE FROM FAILED",19);
/*******返回结果为失败的初始化过程**********************************************/

	if(!str_compare_ignore_case("delete",sql,6))return result;
	sql = sql + 6;
	if(*sql != ' ')return result;                         //判断 第一个关键字和 第二个关键字之间是否有空格，没有就报错。
	while(*sql == ' ') sql++;                             //清除多余的空格
	if(!str_compare_ignore_case("from",sql,4))return result;
	sql = sql + 4;
	if(*sql != ' ')return result;                         //判断 第二个关键字和 table_name之间是否有空格，没有就报错。
	while(*sql == ' ') sql++;                             //清除多余的空格
/********************获取 table_name **************************/
    temp = sub_str(sql," ",1);                           //table_name只能以 空格 结尾
	if(temp==-1)return result;
	
	/**********判断table_name为名字的数据表是否存在*************/
	table = list;
	while(table!=0){
		if(str_compare(table->name,sql,temp) && *(table->name+temp)==0){
			break;
		}
		table = table->next;
	}
	if(table == 0) return result;                            //不存在
	sql += temp;                                             //这句话只能放在这儿，因为上面的while循环利用sql时，sql要指向数据表名字的头

	while(*sql == ' ') sql++;                                //清除多余的空格
	if(!str_compare_ignore_case("where",sql,5))return result;
	sql = sql + 5;
	if(*sql != ' ') return result;                           //where后面 必须有一个 空格
	while(*sql == ' ') sql++;                                //清除多余的空格
/********************获取 where条件判断时，列的名字 **************************/
	temp = sub_str(sql," =",2);                             //列的名字后面跟着空格或者等于号
	if(temp==-1)return result;
	for(i=0;i<table->col_size;i++){
		if(str_compare(table->col[i].name,sql,temp)){
			break;
		}
	}
	if( i == table->col_size) return result;               //列不存在，返回错误
	sql += temp;
	while(*sql == ' ') sql++;                              //清除多余的空格
	if( *sql != '=') return result;                        //列名后面必须有 等于号
	sql++;                                                 //跳过等于号
	while(*sql == ' ') sql++;                              //清除多余的空格
/********************获取列的值 **************************/
	temp=0;
	while(*(sql+temp)!=' ' && *(sql+temp)!=';' && *(sql+temp)!=0)temp++;
	if(temp==0)return result;
	if(table->col[i].type==0){                             //由于 i 是从上面for循环中跳出来的，所以 i 代表列名所指向的那个列
		    element = table->index;
			previous = 0;
			/**************将字符串转换为数字***********/
			memcpy(cache,sql,temp);                       
			*(cache + temp + 1) = 0;
			sscanf_num = sscanf(cache,"%d",&column_content0);
			if(sscanf_num != 1)return result;
			sql += temp;
			/**************将字符串转换为数字***********/
			while(element != 0){
				if(element->content[i] == column_content0){  //在table里面有一个满足条件的element，就删掉一个，所以if满足条件，也不会跳出
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
	else{                                    //另一种类型，text
		    element = table->index;
			previous = 0;
			if(*sql != '\'' ||*(sql+temp-1) != '\'' ) return result;
			/*******************字符串处理************************/
			column_content1 =  (char*)malloc(sizeof(char)*(temp-1));			
			memcpy(column_content1,sql+1,temp-2);                        //把字符串临时存到column_content1里面，第一个单引号，和最后一个单引号去掉
			*(column_content1+temp-2) = 0;			
			sql += temp;
			/*******************字符串处理************************/

			while(element != 0){
				if(str_compare( (char*)(element->content[i]),column_content1,temp-2)){ //在table里面有一个满足条件的element，就删掉一个，所以if满足条件，也不会跳出
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

	while(*sql == ' ') sql++;                             //清除多余的空格
	if(*sql != ';'){                                      //缺少 ; 语法错误
		if(column_content1 != 0)free(column_content1);
		return result;
	}
	if(column_content1 != 0)free(column_content1);        //别忘了释放 临时缓存字符串的指针
	memcpy(result->result,"DELETE FROM SUCCESS",20);
	return result;
}

RecordSet sql_execute(const char *sql)
{
	RecordSet result;
	int temp = 0;
	while(*sql == ' ')  //先排除空格
	    sql++;


	switch(*sql){
	case's':
		return select_execute(sql);
	case'S':
		return select_execute(sql);
	case'c':
		return create_execute(sql);
	case'C':
		return create_execute(sql);
	case't':
		temp = 5;
		while(*(sql+temp) == ' ')temp++;
		if(*(sql+temp) == 'c' || *(sql+temp) == 'C')return create_execute(sql);
		else return drop_execute(sql);
	case'T':
		temp = 5;
		while(*(sql+temp) == ' ')temp++;
		if(*(sql+temp) == 'c' || *(sql+temp) == 'C')return create_execute(sql);
		else return drop_execute(sql);
	case'f':
		temp = 4;
		while(*(sql+temp) == ' ')temp++;
		if(*(sql+temp) == 's' || *(sql+temp) == 'S')return select_execute(sql);
		else return delete_execute(sql);
	case'F':
		temp = 4;
		while(*(sql+temp) == ' ')temp++;
		if(*(sql+temp) == 's' || *(sql+temp) == 'S')return select_execute(sql);
		else return delete_execute(sql);
	case'i':
		return insert_execute(sql);
	case'I':
		return insert_execute(sql);
	case'D':
		if(*(sql+1)== 'r' || *(sql+1)== 'R')return drop_execute(sql);
        return delete_execute(sql);
	case'd':
		if(*(sql+1)== 'r' || *(sql+1)== 'R')return drop_execute(sql);
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
	Record result = (db_record*)malloc(sizeof(struct db_record));
	RecordSet temp = rs;
	index--;
	while(index>=0){
		if(temp == 0){
			result->error = 1;
			result->result[0]=0;
			return result;
		}
		index--;
		temp = temp->next;
	}
	if(temp==0){
		result->error = 1;
		return result;
	}
	if(temp == 0){
			result->error = 1;
			result->result[0]=0;
			return result;
	}
	result->error = 0;
	memcpy(result->result,temp->result,1024);
	return result;
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
	char* result = rcd->result;
	return result;
}


