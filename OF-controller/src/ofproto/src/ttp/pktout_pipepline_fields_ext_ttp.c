/** Table type pattern sd router source file 
 * Copyright (c) 2012, 2013  Freescale.
 *  
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

/* File  :      pktout_pipeline_fields_ttp.c
 * It is a application that used to prototype EXT 427. This extensions 
 * purpose is to send pipeline fields received by contrroler back to switch  
 *
 * Basically giving support in PKT OUT message to carry pipeline fields.
 *
 * This will take metadta pipeline field as example
 */

/* Header files */
#include "controller.h"
#include "pktout_pipeline_fields_ttp.h"
#include "nicira_ext.h"
#include "cntlr_event.h"
#include "ttpldef.h"
#include "cntrlappcmn.h"


extern struct cntlr_ttp_entry ttp_info_g[];
extern void *ttp_table_mempool_g;


int32_t ext427_ttp_init()
{
  int32_t ret_val,ttp_index=-1;
  uint32_t table_id;
  uint8_t heap_b;
  struct  ttp_table_entry *ttp_table_node_p=NULL;

  OF_LOG_MSG(OF_LOG_TTP, OF_LOG_ERROR, "Entered");
  ret_val = get_free_ttp_node_index(&ttp_index);
  if(ret_val!=CNTLR_TTP_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_TTP, OF_LOG_ERROR, "Failed to get free ttp node index.");
    return CNTLR_TTP_FAILURE;
  }
  strcpy(ttp_info_g[ttp_index].ttp_name,
         PKTOUT_PIPELINE_FILEDS_EXT_TTP_NAME);
  OF_LIST_INIT(ttp_info_g[ttp_index].table_list,ttp_free_table_entry_rcu);
  OF_LIST_INIT(ttp_info_g[ttp_index].domain_list,ttp_free_domain_entry_rcu);
  ttp_info_g[ttp_index].is_ttp_cfg=TRUE;


  for(table_id=0; table_id<PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLES_CNT; table_id++)
  {
    /** Create memory for the table */
    ret_val = mempool_get_mem_block(ttp_table_mempool_g, (uchar8_t **)&ttp_table_node_p, &heap_b);
    if(ret_val != MEMPOOL_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_TTP, OF_LOG_ERROR, "memory block allocation failed .");
      ret_val = CNTLR_TTP_FAILURE;
      goto PKTOUT_PIPELINE_FILEDS_TTP_INIT_EXIT;
    }
    ttp_table_node_p->heap_b = heap_b;    
    OF_LIST_INIT(ttp_table_node_p->match_fields_list,ttp_free_table_match_field_entry_rcu);
    switch(table_id)
    {
        case PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID0:
        ttp_table_node_p->table_id = PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID0;
        strcpy(ttp_table_node_p->table_name, PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID0_NAME);
        ttp_table_node_p->max_columns    = PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID0_COLUMNS;
        ttp_table_node_p->max_rows       = PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID0_ROWS;
        ttp_table_node_p->is_wc_entry_present=FALSE;
        ttp_table_node_p->is_lpm_entry_present=FALSE;
        ttp_table_node_p->max_wcs=0;
        ttp_table_node_p->max_lpms=0;

        ret_val = add_match_field_to_table(&(ttp_table_node_p->match_fields_list),
				    OXM_OF_IN_PORT, 
                                    ttp_table_node_p->is_wc_entry_present,
                                    ttp_table_node_p->is_lpm_entry_present);
        if(ret_val != MEMPOOL_SUCCESS)
        {
	  OF_LOG_MSG(OF_LOG_TTP, OF_LOG_ERROR, 
             "Add inport match field to table 0 failed.");
	  ret_val = CNTLR_TTP_FAILURE;
	  goto PKTOUT_PIPELINE_FILEDS_TTP_INIT_EXIT;
        } 
        ret_val = add_match_field_to_table(&(ttp_table_node_p->match_fields_list),
				    OXM_OF_ETH_TYPE, 
                                    ttp_table_node_p->is_wc_entry_present,
                                    ttp_table_node_p->is_lpm_entry_present);
        if(ret_val != MEMPOOL_SUCCESS)
        {
	  OF_LOG_MSG(OF_LOG_TTP, OF_LOG_ERROR, 
             "Add etthtype as IP proto match field to table 0 failed.");
	  ret_val = CNTLR_TTP_FAILURE;
	  goto PKTOUT_PIPELINE_FILEDS_TTP_INIT_EXIT;
        } 
        ret_val = add_match_field_to_table(&(ttp_table_node_p->match_fields_list),
				    OXM_OF_IP_PROTO, 
                                    ttp_table_node_p->is_wc_entry_present,
                                    ttp_table_node_p->is_lpm_entry_present);
        if(ret_val != MEMPOOL_SUCCESS)
        {
	  OF_LOG_MSG(OF_LOG_TTP, OF_LOG_ERROR, 
             "Add etthtype as IP proto match field to table 0 failed.");
	  ret_val = CNTLR_TTP_FAILURE;
	  goto PKTOUT_PIPELINE_FILEDS_TTP_INIT_EXIT;
        } 
	break;	
        case PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID1:
        ttp_table_node_p->table_id = PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID1;
        strcpy(ttp_table_node_p->table_name, PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID1_NAME);
        ttp_table_node_p->max_columns    = PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID1_COLUMNS;
        ttp_table_node_p->max_rows       = PKTOUT_PIPELINE_FILEDS_EXT_TTP_TABLE_ID1_ROWS;
        ttp_table_node_p->is_wc_entry_present=FALSE;
        ttp_table_node_p->is_lpm_entry_present=FALSE;
        ttp_table_node_p->max_wcs=0;
        ttp_table_node_p->max_lpms=0;

        ret_val = add_match_field_to_table(&(ttp_table_node_p->match_fields_list),
				    OXM_OF_METADATA, 
                                    ttp_table_node_p->is_wc_entry_present,
                                    ttp_table_node_p->is_lpm_entry_present);
        if(ret_val != MEMPOOL_SUCCESS)
        {
	  OF_LOG_MSG(OF_LOG_TTP, OF_LOG_ERROR, 
             "Add metadata match field to table 1 failed.");
	  ret_val = CNTLR_TTP_FAILURE;
	  goto PKTOUT_PIPELINE_FILEDS_TTP_INIT_EXIT;
        }
	break;	
     }
     OF_APPEND_NODE_TO_LIST(ttp_info_g[ttp_index].table_list, ttp_table_node_p, 
                                                CNTLR_TTP_TABLE_LIST_NODE_OFFSET);
    ttp_info_g[ttp_index].number_of_tables++;
  }
  return CNTLR_TTP_SUCCESS;
PKTOUT_PIPELINE_FILEDS_TTP_INIT_EXIT:
  ttp_tbl_match_field_list_deinit(PKTOUT_PIPELINE_FILEDS_EXT_TTP_NAME);
  return ret_val;
}

void ext427_ttp_deinit()
{
  ttp_tbl_match_field_list_deinit(PKTOUT_PIPELINE_FILEDS_EXT_TTP_NAME);
}



