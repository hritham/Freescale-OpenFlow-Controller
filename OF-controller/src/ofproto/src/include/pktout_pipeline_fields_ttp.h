/**SAMPLE_L3_FWD_TTP: Following macros are used to proottype 
   PKTOUT metadata extensionsa EXT 427
*/

#define PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLES_CNT 2 /* Number of tables is 2*/

#define PKTOUT_PIPELINE_FILEDS_EXT_TTP_NAME "PKTOUT_PIPELINE_FIELD_EXT_TTP"

#define PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID0        0
#define PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID0_NAME "Table0"
/* 3 match fields OXM_OF_IN_PORT,OXM_OF_ETH_TYPE & OXM_OF_IP_PROTO */
#define PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID0_COLUMNS 3
#define PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID0_ROWS    100

#define PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID1        1
#define PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID1_NAME "Table1"
/*1 match field OXM_OF_METADATA */ 
#define PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID1_COLUMNS 1
#define PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID1_ROWS    100

