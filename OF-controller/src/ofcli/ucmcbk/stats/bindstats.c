/* 
*
* Copyright  2012, 2013  Freescale Semiconductor
*
*
* Licensed under the Apache License, version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at:
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
*     Unless required by applicable law or agreed to in writing, software
*     distributed under the License is distributed on an "AS IS" BASIS,
*     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*     See the License for the specific language governing permissions and
*     limitations under the License.
 */

/*
*
* File name: bindstats.c
* Author: P Vinod Sarma <B46178@freescale.com>
* Date:   03/13/2013
* Description: Bind Statistics.
* 
 */

#ifdef OF_CM_SUPPORT


#include "cbcmninc.h"
#include "cmgutil.h"
#include "cntrucminc.h"
#include "of_utilities.h"
#include "of_multipart.h"

int32_t of_bindstats_appl_ucmcbk_init (void);

int32_t of_bindstats_addrec (void * config_trans_p,
    struct cm_array_of_iv_pairs * pMandParams,
    struct cm_array_of_iv_pairs * pOptParams,
    struct cm_app_result ** pResult);

int32_t of_bindstats_setrec (void * config_trans_p,
    struct cm_array_of_iv_pairs * pMandParams,
    struct cm_array_of_iv_pairs * pOptParams,
    struct cm_app_result ** pResult);

int32_t of_bindstats_delrec (void * pConfigTransction,
    struct cm_array_of_iv_pairs * pKeysArr,
    struct cm_app_result ** pResult);

int32_t of_bindstats_getfirstnrecs (struct cm_array_of_iv_pairs * pKeysArr,
    uint32_t * pCount,
    struct cm_array_of_iv_pairs ** pArrayofIvPairArr);

int32_t of_bindstats_getnextnrecs (struct cm_array_of_iv_pairs * pKeysArr,
    struct cm_array_of_iv_pairs * pPrevRecordKey, uint32_t * pCount,
    struct cm_array_of_iv_pairs ** pNextNRecordData_p);

int32_t of_bindstats_getexactrec (struct cm_array_of_iv_pairs * pKeysArr,
    struct cm_array_of_iv_pairs ** pIvPairArr);

int32_t of_bindstats_verifycfg (struct cm_array_of_iv_pairs * pKeysArr,
    uint32_t command_id, struct cm_app_result ** pResult);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* * * * * * * V L A N  U T I L I T Y  F U N C T I O N S * * * * * * * * * * *
* * * * * * * * * * * * * **  * * * * * * * **  * * * * * * * * * * * * * */
int32_t of_bindstats_ucm_validatemandparams (struct cm_array_of_iv_pairs *
    pMandParams,
    struct cm_app_result **ppResult);

int32_t of_bindstats_ucm_validateoptparams (struct cm_array_of_iv_pairs *
    pOptParams,
    struct cm_app_result ** ppResult);

int32_t of_bindstats_ucm_setmandparams (struct cm_array_of_iv_pairs *
    pMandParams,
    struct dprm_datapath_general_info *datapath_info,
    struct ofi_bind_entry_info* stats_info,
    struct cm_app_result ** ppResult);

int32_t of_bindstats_ucm_setoptparams (struct cm_array_of_iv_pairs *
    pOptParams,
    struct ofi_bind_entry_info * stats_info,
    struct cm_app_result ** ppResult);


int32_t of_bindstats_ucm_getparams (struct ofi_bind_entry_info *stats_info,
    struct cm_array_of_iv_pairs * result_iv_pairs_p);


struct cm_dm_app_callbacks of_bindstats_ucm_callbacks = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  of_bindstats_getfirstnrecs,
  of_bindstats_getnextnrecs,
  of_bindstats_getexactrec,
  NULL,
  NULL
};
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* * * * * * * * * * * * Switch  UCM Init * * * * * * *  * * * * * * * * * * *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int32_t of_bindstats_appl_ucmcbk_init (void)
{
  CM_CBK_DEBUG_PRINT ("Entry");
  cm_register_app_callbacks (CM_DM_STATS_BINDSTATS_ID,&of_bindstats_ucm_callbacks);
  return OF_SUCCESS;
}



int32_t of_bindstats_getfirstnrecs (struct cm_array_of_iv_pairs * pKeysArr,
    uint32_t * pCount,
    struct cm_array_of_iv_pairs ** pArrayofIvPairArr)
{
  struct cm_array_of_iv_pairs *result_iv_pairs_p = NULL;
  int32_t return_value = OF_FAILURE;
  uint32_t uiRecCount = 0;
  struct cm_app_result *stats_result = NULL;
  struct dprm_datapath_general_info datapath_info={};
  struct ofi_bind_entry_info stats_info;
  struct dprm_port_runtime_info runtime_info;
  uint64_t datapath_handle;
  uint64_t port_handle;

  of_memset (&stats_info, 0, sizeof (struct ofi_bind_entry_info));

  if ((of_bindstats_ucm_setmandparams (pKeysArr,&datapath_info, &stats_info, &stats_result)) !=
      OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    return OF_FAILURE;
  }

  return_value=dprm_get_datapath_handle(datapath_info.dpid, &datapath_handle);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Get DPRM Handle failed");
    return OF_FAILURE;
  }

  return_value=cntlr_send_bind_stats_request(datapath_handle); 
  if ( return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("bindstats failed");
    return OF_FAILURE;
  } 
  CM_CBK_DEBUG_PRINT ("bind stats will be sent as multipart response");
  return CM_CNTRL_MULTIPART_RESPONSE;
}


int32_t of_bindstats_getnextnrecs (struct cm_array_of_iv_pairs * pKeysArr,
    struct cm_array_of_iv_pairs *pPrevRecordKey, uint32_t * pCount,
    struct cm_array_of_iv_pairs ** pNextNRecordData_p)
{

  struct cm_array_of_iv_pairs *result_iv_pairs_p = NULL;
  struct cm_app_result *stats_result = NULL;
  int32_t return_value = OF_FAILURE;

  CM_CBK_DEBUG_PRINT ("Entered");
  *pCount=0;
  return return_value;

}

int32_t of_bindstats_getexactrec (struct cm_array_of_iv_pairs * pKeysArr,
    struct cm_array_of_iv_pairs ** pIvPairArr)
{
  struct cm_array_of_iv_pairs *result_iv_pairs_p = NULL;
  struct cm_app_result *stats_result = NULL;
  int32_t return_value = OF_FAILURE;

  CM_CBK_DEBUG_PRINT ("Entered");
  return return_value;

}

int32_t of_bindstats_verifycfg (struct cm_array_of_iv_pairs * pKeysArr,
    uint32_t command_id, struct cm_app_result ** pResult)
{
  struct cm_app_result *stats_result = NULL;
  int32_t return_value = OF_FAILURE;
  struct ofi_bind_entry_info stats_info = { };

  CM_CBK_DEBUG_PRINT ("Entered");
  of_memset (&stats_info, 0, sizeof (struct ofi_bind_entry_info));

  return_value = of_bindstats_ucm_validatemandparams (pKeysArr, &stats_result);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Validate Mandatory Parameters Failed");
    return OF_FAILURE;
  }

  if (datapath_ucm_validateoptparams (pKeysArr, &stats_result)
      != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Validate Mandatory Parameters Failed");
    return OF_FAILURE;
  }

  *pResult = stats_result;
  return OF_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* * * * * * * V L A N  U T I L I T Y  F U N C T I O N S * * * * * * * * * * *
* * * * * * * * * * * * * **  * * * * * * * **  * * * * * * * * * * * * * */

int32_t of_bindstats_ucm_validatemandparams (struct cm_array_of_iv_pairs *
    pMandParams,	struct cm_app_result **ppResult)
{
  uint32_t uiMandParamCnt;
  struct cm_app_result *stats_result = NULL;
  uint32_t uiPortId;


  CM_CBK_DEBUG_PRINT ("Entered");
  for (uiMandParamCnt = 0; uiMandParamCnt < pMandParams->count_ui;
      uiMandParamCnt++)
  {
    switch (pMandParams->iv_pairs[uiMandParamCnt].id_ui)
    {
      case CM_DM_BINDSTATS_BIND_OBJ_ID_ID:  
        if (pMandParams->iv_pairs[uiMandParamCnt].value_p == NULL)
        {
          CM_CBK_DEBUG_PRINT ("bind obj ID is NULL");
          fill_app_result_struct (&stats_result, NULL, CM_GLU_TABLE_ID_NULL);
          *ppResult = stats_result;
          return OF_FAILURE;
        }
        break;

    }
  }
  CM_CBK_PRINT_IVPAIR_ARRAY (pMandParams);
  return OF_SUCCESS;
}

int32_t of_bindstats_ucm_validateoptparams (struct cm_array_of_iv_pairs *
    pOptParams,
    struct cm_app_result ** ppResult)
{
  uint32_t uiOptParamCnt, uiTableCnt;

  CM_CBK_DEBUG_PRINT ("Entered");
  for (uiOptParamCnt = 0; uiOptParamCnt < pOptParams->count_ui; uiOptParamCnt++)
  {
    switch (pOptParams->iv_pairs[uiOptParamCnt].id_ui)
    {
      default:
        break;
    }
  }
  CM_CBK_PRINT_IVPAIR_ARRAY (pOptParams);
  return OF_SUCCESS;
}

int32_t of_bindstats_ucm_setmandparams (struct cm_array_of_iv_pairs *
    pMandParams,
    struct dprm_datapath_general_info *datapath_info,
    struct ofi_bind_entry_info* stats_info,		struct cm_app_result ** ppResult)
{
  uint32_t uiMandParamCnt;
  uint64_t idpId;

  CM_CBK_DEBUG_PRINT ("Entered");
  for (uiMandParamCnt = 0; uiMandParamCnt < pMandParams->count_ui;
      uiMandParamCnt++)
  {
    switch (pMandParams->iv_pairs[uiMandParamCnt].id_ui)
    {
      case CM_DM_DATAPATH_DATAPATHID_ID:
        idpId=charTo64bitNum((char *) pMandParams->iv_pairs[uiMandParamCnt].value_p);
        datapath_info->dpid=idpId;
        CM_CBK_DEBUG_PRINT("dpid is: %llx\r\n", idpId);
        break;
#if 0
      case CM_DM_FLOWSTATS_TABLEID_ID: 
        stats_info->table_id = of_atoi((char *) pMandParams->iv_pairs[uiMandParamCnt].value_p);
        CM_CBK_DEBUG_PRINT ("Table ID: %d",stats_info->table_id);
        break;
#endif
    }
  }
  CM_CBK_PRINT_IVPAIR_ARRAY (pMandParams);
  return OF_SUCCESS;
}

int32_t of_bindstats_ucm_setoptparams (struct cm_array_of_iv_pairs *
    pOptParams,
    struct ofi_bind_entry_info * stats_info,
    struct cm_app_result ** ppResult)
{
  uint32_t uiOptParamCnt;

  CM_CBK_DEBUG_PRINT ("Entered");

  for (uiOptParamCnt = 0; uiOptParamCnt < pOptParams->count_ui; uiOptParamCnt++)
  {
    switch (pOptParams->iv_pairs[uiOptParamCnt].id_ui)
    {
      default:
        break;
    }
  }

  CM_CBK_PRINT_IVPAIR_ARRAY (pOptParams);
  return OF_SUCCESS;
}


int32_t of_bindstats_ucm_getparams (struct ofi_bind_entry_info *stats_info,
    struct cm_array_of_iv_pairs * result_iv_pairs_p)
{
  char *instruction_actions[]={
    "OFPAT_OUTPUT",
    "None",
    "None",
    "None",
    "None",
    "None",
    "None",
    "None",
    "None",
    "None",
    "None",
    "OFPAT_COPY_TTL_OUT",
    "OFPAT_COPY_TTL_IN",
    "None",
    "None",
    "OFPAT_SET_MPLS_TTL",
    "OFPAT_DEC_MPLS_TTL",
    "OFPAT_PUSH_VLAN",
    "OFPAT_POP_VLAN",
    "OFPAT_PUSH_MPLS",
    "OFPAT_POP_MPLS",
    "OFPAT_SET_QUEUE",
    "OFPAT_GROUP",
    "OFPAT_SET_NW_TTL",
    "OFPAT_DEC_NW_TTL",
    "OFPAT_SET_FIELD",
    "OFPAT_PUSH_PBB",
    "OFPAT_POP_PBB"
  };

  char *port_type[]={
    "OFPP_MAX",
    "OFPP_IN_PORT",
    "OFPP_TABLE",
    "OFPP_NORMAL",
    "OFPP_FLOOD",
    "OFPP_ALL",
    "OFPP_CONTROLLER",
    "OFPP_LOCAL",
    "OFPP_ANY"
  };

char *fslx_instrctn_subtype[]= 
{
  "None",
  "FSLX_APPLY_ACTIONS_ON_CREATE",
  "FSLX_APPLY_ACTIONS_ON_DELETE",
  "FSLXIT_WAIT_TO_COMPLETE_PKT_OUTS",
  "FSLXIT_JUMP",
  "FSLXIT_WRITE_PRIORITY_REGISTER",
  "FSLXIT_WRITE_PRIORITY_REG_FROM_CURRENT_FLOW",
  "FSLXIT_RE_LOOKUP",
  "FSLXIT_RE_INJECT_TO",
  "FSLXIT_APPLY_GROUP",
  "FSLXIT_WRITE_GROUP",
  "EXT1",
  "EXT2",
  "EXT3",
  "EXT4"
};

char *fslx_action_subtype[]=
{
    "None",
    "FSLXAT_ATTACH_BIND_OBJ",
    "FSLXAT_DETACH_BIND_OBJ",            
    "FSLXAT_ATTACH_REVALIDATION_OBJ",
    "FSLXAT_DETACH_REVALIDATION_OBJ",
    "FSLXAT_SET_FLOW_DIRECTION",          
    "FSLXAT_SET_APP_STATE_INFO",          
    "FSLXAT_SET_CLOSE_WAIT_TIME",         
    "FSLXAT_SEND_APP_STATE_INFO",         
    "FSLXAT_BIND_OBJECT_APPLY_ACTIONS",
    "FSLXAT_BIND_OBJECT_WRITE_ACTIONS",
    "FSLXAT_SET_PHY_PORT_FIELD_CNTXT",
    "FSLXAT_SET_META_DATA_FIELD_CNTXT",
    "FSLXAT_SET_TUNNEL_ID_FIELD_CNTXT",
    "FSLXAT_ENABLE_TCP_TERMINATE_SM",
    "FSLXAT_ARP_RESPONSE",
    "FSLXAT_WRITE_METADATA_FROM_PKT",
    "FSLXAT_SEQ_NO_DELTA_ADDITION",
    "FSLXAT_SEQ_NO_VALIDATION",
    "FSLXAT_START_TABLE",
    "FSLXAT_IP_REASM",
    "FSLXAT_IP_FRAGMENT",
    "FSLXAT_SEND_ICMPV4_TIME_EXCEED",
    "FSLXAT_SEND_ICMPV4_DEST_UNREACHABLE",
    "FSLXAT_DROP_PACKET",
    "FSLXAT_CREATE_SA_CNTXT", 
    "FSLXAT_DELETE_SA_CNTXT",
    "FSLXAT_IPSEC_ENCAP",   
    "FSLXAT_IPSEC_DECAP",
    "FSLXAT_POP_NATT_UDP_HEADER",
    "FSLXAT_WRITE_META_DATA",
    "EXT1",
    "EXT2",
    "EXT3",
    "EXT4"
};

  char string[1024];
  char tmp_string[1024];
  char buf_port[40];
  int32_t index = 0, jj, count_ui, iIndex;
  uint8_t bind_obj_id;  
  uint8_t table_id;  
  uint64_t metadata; 
  uint64_t metadata_mask; 
  int32_t no_of_actions, i;
  struct ofi_action *action_entry_p;
  struct ofi_instruction *instruction_entry_p;
  struct ofi_match_field *match_entry_p;

#define CM_BINDSTATS_CHILD_COUNT 200
  count_ui = CM_BINDSTATS_CHILD_COUNT;

  result_iv_pairs_p->iv_pairs = (struct cm_iv_pair*)of_calloc(count_ui, sizeof(struct cm_iv_pair));
  if(!result_iv_pairs_p->iv_pairs)
  {
    CM_CBK_DEBUG_PRINT (" Memory allocation failed ");
    return OF_FAILURE;
  }
  of_memset(string, 0, sizeof(string));
  sprintf(string,"%d",stats_info->bind_obj_id);
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_BINDSTATS_BIND_OBJ_ID_ID,CM_DATA_TYPE_STRING, string);
  index++;

  of_memset(string, 0, sizeof(string));
  sprintf(string,"%d",stats_info->type);
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_BINDSTATS_APPLICATION_TYPE_ID,CM_DATA_TYPE_STRING, string);
  index++;
/*
  of_memset(string, 0, sizeof(string));
  of_itoa(stats_info->alive_sec,string);
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],,CM_DATA_TYPE_STRING, string);
  index++;

  of_memset(string, 0, sizeof(string));
  of_itoa(stats_info->alive_nsec,string);
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_FLOWSTATS_ALIVENSEC_ID,CM_DATA_TYPE_STRING, string);
  index++;

  of_memset(string, 0, sizeof(string));
  of_ltoa(stats_info->cookie,string);
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_FLOWSTATS_COOKIE_ID,CM_DATA_TYPE_STRING, string);
  index++;

  of_memset(string, 0, sizeof(string));
  sprintf(string,"%d",stats_info->Idle_timeout);
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_FLOWSTATS_IDLETIMEOUT_ID,CM_DATA_TYPE_STRING, string);
  index++;

  of_memset(string, 0, sizeof(string));
  sprintf(string,"%d",stats_info->hard_timeout);
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_FLOWSTATS_HARDTIMEOUT_ID,CM_DATA_TYPE_STRING, string);
  index++;
*/
  of_memset(string, 0, sizeof(string));
  of_ltoa (stats_info->packet_count, string);
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_BINDSTATS_PACKETCOUNT_ID,CM_DATA_TYPE_STRING, string);
  index++;

  of_memset(string, 0, sizeof(string));
  of_ltoa(stats_info->byte_count,string);
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_BINDSTATS_BYTECOUNT_ID,CM_DATA_TYPE_STRING, string);
  index++;
  
  of_memset(string, 0, sizeof(string));
  of_ltoa(stats_info->error_count,string);
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_BINDSTATS_ERRORCOUNT_ID,CM_DATA_TYPE_STRING, string);
  index++;
/*
  of_memset(string, 0, sizeof(string));
  of_sprintf(string,"%d",OF_LIST_COUNT(stats_info->match_field_list));
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_FLOWSTATS_MATCHFIELD_COUNT_ID,CM_DATA_TYPE_STRING, string);
  index++;

  OF_LIST_SCAN(stats_info->match_field_list, match_entry_p, struct ofi_match_field *, OF_MATCH_LISTNODE_OFFSET)
  {
    of_memset(string, 0, sizeof(string));
    switch(match_entry_p->field_type)
    {
      case OFPXMT_OFB_IN_PORT:
        of_sprintf(string,"Type=OFPXMT_OFB_IN_PORT  Value=%d", match_entry_p->ui32_data); 
        break;

      case OFPXMT_OFB_IN_PHY_PORT:
        of_sprintf(string,"Type=OFPXMT_OFB_IN_PHY_PORT  Value=%d",match_entry_p->ui32_data);
        break;

      case OFPXMT_OFB_METADATA:
        of_sprintf(string,"Type=OFPXMT_OFB_METADATA  Value=%llx",match_entry_p->ui64_data);
        break;

      case OFPXMT_OFB_ETH_DST:
        of_sprintf(string,"Type=OFPXMT_OFB_ETH_DST  Value=%02x:%02x:%02x:%02x:%02x:%02x",match_entry_p->ui8_data_array[0],match_entry_p->ui8_data_array[1], match_entry_p->ui8_data_array[2],match_entry_p->ui8_data_array[3],match_entry_p->ui8_data_array[4],match_entry_p->ui8_data_array[5]);
        break;

      case OFPXMT_OFB_ETH_SRC:
        of_sprintf(string,"Type=OFPXMT_OFB_ETH_SRC  Value=%02x:%02x:%02x:%02x:%02x:%02x",match_entry_p->ui8_data_array[0],match_entry_p->ui8_data_array[1],match_entry_p->ui8_data_array[2],match_entry_p->ui8_data_array[3],match_entry_p->ui8_data_array[4],match_entry_p->ui8_data_array[5]);
        break;

      case OFPXMT_OFB_ETH_TYPE:
        of_sprintf(string,"Type=OFPXMT_OFB_ETH_TYPE  Value=%x",match_entry_p->ui16_data);
        break;

      case OFPXMT_OFB_VLAN_VID:
        of_sprintf(string,"Type=OFPXMT_OFB_VLAN_VID  Value=%d",match_entry_p->ui16_data);
        break;

      case OFPXMT_OFB_IP_PROTO:
        of_sprintf(string,"Type=OFPXMT_OFB_IP_PROTO Value=%d",match_entry_p->ui8_data);
        break;

      case OFPXMT_OFB_IPV4_SRC:
        of_sprintf(string,"Type=OFPXMT_OFB_IPV4_SRC Value=%x",match_entry_p->ui32_data);
        break;

      case OFPXMT_OFB_IPV4_DST:
        of_sprintf(string,"Type=OFPXMT_OFB_IPV4_DST Value=%x",match_entry_p->ui32_data);
        break;

      case OFPXMT_OFB_TCP_SRC:
        of_sprintf(string,"Type=OFPXMT_OFB_TCP_SRC Value=%d",match_entry_p->ui16_data);
        break;

      case OFPXMT_OFB_TCP_DST:
        of_sprintf(string,"Type=OFPXMT_OFB_TCP_DST Value=%d",match_entry_p->ui16_data);
        break;

      case OFPXMT_OFB_UDP_SRC:
        of_sprintf(string,"Type=OFPXMT_OFB_UDP_SRC Value=%d",match_entry_p->ui16_data);
        break;

      case OFPXMT_OFB_UDP_DST:
        of_sprintf(string,"Type=OFPXMT_OFB_UDP_DST Value=%d",match_entry_p->ui16_data);
        break;

      case OFPXMT_OFB_SCTP_SRC:
        of_sprintf(string,"Type=OFPXMT_OFB_SCTP_SRC Value=%d",match_entry_p->ui16_data);
        break;

      case OFPXMT_OFB_SCTP_DST:
        of_sprintf(string,"Type=OFPXMT_OFB_SCTP_DST Value=%d",match_entry_p->ui16_data);
        break;

      case OFPXMT_OFB_ICMPV4_TYPE:
        of_sprintf(string,"Type=OFPXMT_OFB_ICMPV4_TYPE Value=%d",match_entry_p->ui8_data);
        break;

      case OFPXMT_OFB_ICMPV4_CODE:
        of_sprintf(string,"Type=OFPXMT_OFB_ICMPV4_CODE Value=%d",match_entry_p->ui8_data);
        break;

      case OFPXMT_OFB_ARP_OP:
        of_sprintf(string,"Type=OFPXMT_OFB_ARP_OP Value=%d",match_entry_p->ui16_data);
        break;

      case OFPXMT_OFB_MPLS_LABEL:
        of_sprintf(string,"Type=OFPXMT_OFB_MPLS_LABEL Value=%d",match_entry_p->ui32_data);
        break;

      case OFPXMT_OFB_MPLS_TC:
        of_sprintf(string,"Type=OFPXMT_OFB_MPLS_TC Value=%d",match_entry_p->ui8_data);
        break;

      case OFPXMT_OFB_MPLS_BOS:
        of_sprintf(string,"Type=OFPXMT_OFB_MPLS_BOS Value=%d",match_entry_p->ui8_data);
        break;

      case OFPXMT_OFB_PBB_ISID:
        of_sprintf(string,"Type=OFPXMT_OFB_PBB_ISID Value=%d",match_entry_p->ui8_data);
        break;

      case OFPXMT_OFB_TUNNEL_ID:
        of_sprintf(string,"Type=OFPXMT_OFB_TUNNEL_ID Value=%llx",match_entry_p->ui64_data);
        break;

      case OFPXMT_OFB_VLAN_PCP:
        of_sprintf(string,"Type=OFPXMT_OFB_VLAN_PCP Value=%d",match_entry_p->ui8_data);
        break;

      case OFPXMT_OFB_ARP_SPA:
        of_sprintf(string,"Type=OFPXMT_OFB_ARP_SPA Value=%x",match_entry_p->ui32_data);
        break;      
      
      case OFPXMT_OFB_ARP_THA:
        of_sprintf(string,"Type=OFPXMT_OFB_ARP_THA Value=%x",match_entry_p->ui32_data);
        break;

      case OFPXMT_OFB_IP_DSCP:
        of_sprintf(string,"Type=OFPXMT_OFB_IP_DSCP Value=%d",match_entry_p->ui8_data);
        break;

      case OFPXMT_OFB_IP_ECN:
        of_sprintf(string,"Type=OFPXMT_OFB_IP_ECN Value=%d",match_entry_p->ui8_data);
        break;

      case OFPXMT_OFB_IPV6_SRC:
        of_sprintf(string,"Type=OFPXMT_OFB_IPV6_SRC");
        break;
  
      case OFPXMT_OFB_IPV6_DST:
        of_sprintf(string,"Type=OFPXMT_OFB_IPV6_DST");
        break;

      case OFPXMT_OFB_ICMPV6_TYPE:
        of_sprintf(string,"Type=OFPXMT_OFB_ICMPV6_TYPE Value=%d",match_entry_p->ui8_data);
        break;

      case OFPXMT_OFB_ICMPV6_CODE:
        of_sprintf(string,"Type=OFPXMT_OFB_ICMPV6_CODE Value=%d",match_entry_p->ui8_data);
        break;

      case OFPXMT_OFB_IPV6_FLABEL:
        of_sprintf(string,"Type=OFPXMT_OFB_IPV6_FLABEL Value=%x",match_entry_p->ui32_data);
        break;

      case OFPXMT_OFB_IPV6_ND_TARGET:
        of_sprintf(string,"Type=OFPXMT_OFB_IPV6_ND_TARGET");
        break;

      case OFPXMT_OFB_IPV6_ND_SLL:
        of_sprintf(string,"Type=OFPXMT_OFB_IPV6_ND_SLL");
        break;

      case OFPXMT_OFB_IPV6_ND_TLL:
        of_sprintf(string,"Type=OFPXMT_OFB_IPV6_ND_TLL");
        break;

      case OFPXMT_OFB_IPV6_EXTHDR:
        of_sprintf(string,"Type=OFPXMT_OFB_IPV6_EXTHDR");
        break;

      default:
        of_sprintf(string,"Type=UNKNOWN");
        break;

    }
    //For diplaying the mask value
    OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"maskset is %d  value  is %x",match_entry_p->is_mask_set,match_entry_p->mask);

    if(match_entry_p->is_mask_set == 1){
    if(match_entry_p->mask_len == 4){
        of_sprintf(string,"%s MASK = %x",string,ntohl(*(uint32_t*)(match_entry_p->mask)));
    }
    else if((match_entry_p->mask_len == 6)){
        of_sprintf(string,"%s  MASK=%02x:%02x:%02x:%02x:%02x:%02x",string,match_entry_p->mask[0],match_entry_p->mask[1],match_entry_p->mask[2],match_entry_p->mask[3],match_entry_p->mask[4],match_entry_p->mask[5]);
    }
    else if(match_entry_p->mask_len == 8){
        of_sprintf(string,"%s MASK IS SET",string);
    }
    else if(match_entry_p->mask_len == 16){
        of_sprintf(string,"%s MASK IS SET",string);
    }
    }


    FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_FLOWSTATS_MATCHFIELDINFO_ID,CM_DATA_TYPE_STRING, string);
    index++;
  }
*/
  of_memset(string, 0, sizeof(string));
  of_sprintf(string,"%d",OF_LIST_COUNT(stats_info->instruction_list));
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_BINDSTATS_INSTRUCTION_COUNT_ID,CM_DATA_TYPE_STRING, string);
  index++;

  OF_LIST_SCAN(stats_info->instruction_list, instruction_entry_p, struct ofi_instruction *,OF_INSTRUCTION_LISTNODE_OFFSET )
  {
    of_memset(string, 0, sizeof(string));
    switch(instruction_entry_p->type)
    {
      case  OFPIT_GOTO_TABLE:
        bind_obj_id = instruction_entry_p->table_id;
        of_sprintf(string,"Type=GOTO_TABLE TableID=%d", table_id); 
        break;

      case OFPIT_WRITE_METADATA:
        metadata = instruction_entry_p->metadata;
        metadata_mask = instruction_entry_p->metadata_mask;
        of_sprintf(string,"Type=WRITE_METADATA Metadata=%llx MetadataMask=%llx", metadata,metadata_mask);
        break;

      case OFPIT_APPLY_ACTIONS:
        of_sprintf(string,"Type=APPLY_ACTIONS ");
        break;

      case OFPIT_WRITE_ACTIONS:
      case OFPIT_CLEAR_ACTIONS:
      case OFPIT_METER:
        of_sprintf(string,"TODO ");
        break;

      case OFPIT_EXPERIMENTER:
        of_sprintf(string,"Type=EXPERIMENTER Vedor ID=%x SubType=%s", instruction_entry_p->vendorid,fslx_instrctn_subtype[instruction_entry_p->subtype]);
        break;

    }
    FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_BINDSTATS_INSTRUCTIONINFO_ID,CM_DATA_TYPE_STRING, string);
    index++;

    of_memset(string, 0, sizeof(string));
    of_sprintf(string,"%d",OF_LIST_COUNT(instruction_entry_p->action_list));
    FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_BINDSTATS_ACTION_COUNT_ID,CM_DATA_TYPE_STRING, string);
    index++;

    no_of_actions=OF_LIST_COUNT(instruction_entry_p->action_list);
    i=0;  
    if ( no_of_actions == 0 )
    {
      of_memset(string, 0, sizeof(string));
      of_sprintf(string,"%s", " - ");
      FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_BINDSTATS_ACTION_COUNT_ID,CM_DATA_TYPE_STRING, string);
      index++;
    }
    else
    {
      of_memset(string, 0, sizeof(string));
      OF_LIST_SCAN(instruction_entry_p->action_list, action_entry_p, struct ofi_action *, OF_ACTION_LISTNODE_OFFSET )
      {
        i++;

        of_memset(tmp_string, 0, sizeof(tmp_string));
        of_memset(buf_port, 0, sizeof(buf_port));
        if(action_entry_p->type != OFPAT_EXPERIMENTER)
          of_sprintf(string,"Type=%s",instruction_actions[action_entry_p->type]);
        switch(action_entry_p->type)
        {
          case   OFPAT_OUTPUT:  /* Output to switch port. */
            {
              of_sprintf(buf_port,"%lx", action_entry_p->port_no);
              if ( (strcmp(buf_port, "ffffff00") == 0))
                of_sprintf( tmp_string," port=%s max_len=%d", port_type[0],action_entry_p->max_len);
              else if ( (strcmp(buf_port, "fffffff8") == 0))
                of_sprintf( tmp_string," port=%s max_len=%d", port_type[1],action_entry_p->max_len);
              else if (strcmp(buf_port, "fffffff9") == 0) 
                of_sprintf( tmp_string," port=%s max_len=%d", port_type[2],action_entry_p->max_len);
              else if (strcmp(buf_port, "fffffffa") == 0)
                of_sprintf( tmp_string," port=%s max_len=%d", port_type[3],action_entry_p->max_len);
              else if (strcmp(buf_port, "fffffffb") == 0)
                of_sprintf( tmp_string," port=%s max_len=%d", port_type[4],action_entry_p->max_len);
              else if (strcmp(buf_port, "fffffffc") == 0)
                of_sprintf( tmp_string," port=%s max_len=%d", port_type[5],action_entry_p->max_len);
              else if (strcmp(buf_port, "fffffffd") == 0)
                of_sprintf( tmp_string," port=%s max_len=%d", port_type[6],action_entry_p->max_len);
              else if (strcmp(buf_port, "fffffffe") == 0)
                of_sprintf( tmp_string," port=%s max_len=%d", port_type[7],action_entry_p->max_len);
              else if (strcmp(buf_port, "ffffffff") == 0)
                of_sprintf( tmp_string," port=%s max_len=%d", port_type[8],action_entry_p->max_len);
              else
                of_sprintf( tmp_string," port=%ld max_len=%d",action_entry_p->port_no,action_entry_p->max_len);
            }
            break;
          case   OFPAT_SET_MPLS_TTL : /* MPLS TTL */
          case   OFPAT_SET_NW_TTL : /* IP TTL. */
            {
              of_sprintf(tmp_string," ttl=%d ",action_entry_p->ttl);
            }
            break;
          case   OFPAT_PUSH_VLAN  : /* Push a new VLAN tag */
          case   OFPAT_PUSH_MPLS: /* Push a new MPLS tag */
          case   OFPAT_POP_MPLS : /* Pop the outer MPLS tag */
          case   OFPAT_PUSH_PBB: /* Push a new PBB service tag (I-TAG) */
            {
              of_sprintf(tmp_string," ether_type=%d ",action_entry_p->ether_type);
            }
            break;
          case   OFPAT_SET_QUEUE: /* Set queue id when outputting to a port */
            {
              of_sprintf(tmp_string," queue_id=%d ",action_entry_p->queue_id);
            }
            break;
          case   OFPAT_GROUP : /* Apply group. */
            {
              of_sprintf(tmp_string," group_id=%d ",action_entry_p->group_id);
            }
            break;
          case   OFPAT_COPY_TTL_OUT: /* Copy TTL "outwards" -- from next-to-outermost    to outermost */
          case   OFPAT_COPY_TTL_IN  : /* Copy TTL "inwards" -- from outermost to    next-to-outermost */
          case   OFPAT_DEC_MPLS_TTL : /* Decrement MPLS TTL */
          case   OFPAT_DEC_NW_TTL: /* Decrement IP TTL. */    
          case   OFPAT_POP_PBB: /* Pop the outer PBB service tag (I-TAG) */
            {
              of_sprintf(tmp_string,"%s", " - ");
            }
            break;
          case OFPAT_SET_FIELD: 
            {
              OF_LOG_MSG(OF_LOG_MOD,OF_LOG_DEBUG,"field type is %d",action_entry_p->setfield_type);
              switch(action_entry_p->setfield_type){
                case OFPXMT_OFB_IN_PORT:
                  of_sprintf(tmp_string," OFPXMT_OFB_IN_PORT= %d",action_entry_p->ui32_data);
                  break;
                case OFPXMT_OFB_IN_PHY_PORT:
                  of_sprintf(tmp_string," OFPXMT_OFB_IN_PHY_PORT:= %d",action_entry_p->ui32_data);
                  break;
                case OFPXMT_OFB_METADATA: 
                  of_sprintf(tmp_string," OFPXMT_OFB_METADATA:= %d",action_entry_p->ui64_data);
                  break;
                case OFPXMT_OFB_ETH_DST:
                  of_sprintf(string,"Type=OFPXMT_OFB_ETH_DST  Value=%02x:%02x:%02x:%02x:%02x:%02x",action_entry_p->ui8_data_array[0],action_entry_p->ui8_data_array[1], action_entry_p->ui8_data_array[2],action_entry_p->ui8_data_array[3],action_entry_p->ui8_data_array[4],action_entry_p->ui8_data_array[5]);
                  break;
                case OFPXMT_OFB_ETH_SRC:
                  of_sprintf(string,"Type=OFPXMT_OFB_ETH_DST  Value=%02x:%02x:%02x:%02x:%02x:%02x",action_entry_p->ui8_data_array[0],action_entry_p->ui8_data_array[1], action_entry_p->ui8_data_array[2],action_entry_p->ui8_data_array[3],action_entry_p->ui8_data_array[4],action_entry_p->ui8_data_array[5]);
                  break;
                case OFPXMT_OFB_ETH_TYPE:
                  of_sprintf(tmp_string," OFPXMT_OFB_ETH_TYPE:= %d",action_entry_p->ui16_data);
                  break;
                case OFPXMT_OFB_VLAN_VID:
                  of_sprintf(tmp_string," OFPXMT_OFB_VLAN_VID:= %d",action_entry_p->ui16_data);
                  break;
                case OFPXMT_OFB_IP_PROTO:
                  of_sprintf(tmp_string," OFPXMT_OFB_IP_PROTO:= %d",action_entry_p->ui8_data);
                  break;
                case OFPXMT_OFB_IPV4_SRC:
                  of_sprintf(tmp_string," OFPXMT_OFB_IPV4_SRC:= %x",action_entry_p->ui32_data);
                  break;
                case OFPXMT_OFB_IPV4_DST:
                  of_sprintf(tmp_string," OFPXMT_OFB_IPV4_DST:= %x",action_entry_p->ui32_data);
                  break;
                case OFPXMT_OFB_TCP_SRC:
                  of_sprintf(tmp_string," OFPXMT_OFB_TCP_SRC:= %d",action_entry_p->ui16_data);
                  break;
                case OFPXMT_OFB_TCP_DST:
                  of_sprintf(tmp_string," OFPXMT_OFB_TCP_DST:= %d",action_entry_p->ui16_data);
                  break;
                case OFPXMT_OFB_UDP_SRC:
                  of_sprintf(tmp_string," OFPXMT_OFB_UDP_SRC:= %d",action_entry_p->ui16_data);
                  break;
                case OFPXMT_OFB_UDP_DST:
                  of_sprintf(tmp_string," OFPXMT_OFB_UDP_DST:= %d",action_entry_p->ui16_data);
                  break;
                case OFPXMT_OFB_SCTP_SRC:
                  of_sprintf(tmp_string," OFPXMT_OFB_SCTP_SRC:= %d",action_entry_p->ui16_data);
                  break;
                case OFPXMT_OFB_SCTP_DST:
                  of_sprintf(tmp_string," OFPXMT_OFB_SCTP_DST:= %d",action_entry_p->ui16_data);
                  break;
                case OFPXMT_OFB_ICMPV4_TYPE:
                  of_sprintf(tmp_string," OFPXMT_OFB_ICMPV4_TYPE:= %d",action_entry_p->ui8_data);
                  break;
                case OFPXMT_OFB_ICMPV4_CODE:
                  of_sprintf(tmp_string," OFPXMT_OFB_ICMPV4_CODE:= %d",action_entry_p->ui8_data);
                  break;
                case OFPXMT_OFB_ARP_OP:
                  of_sprintf(tmp_string," OFPXMT_OFB_ARP_OP:= %d",action_entry_p->ui16_data);
                  break;
                case OFPXMT_OFB_MPLS_LABEL:
                  of_sprintf(tmp_string," OFPXMT_OFB_MPLS_LABEL:= %d",action_entry_p->ui32_data);
                  break;
                case OFPXMT_OFB_MPLS_TC:
                  of_sprintf(tmp_string," OFPXMT_OFB_MPLS_TC:= %d",action_entry_p->ui8_data);
                  break;
                case OFPXMT_OFB_MPLS_BOS:
                  of_sprintf(tmp_string," OFPXMT_OFB_MPLS_BOS:= %d",action_entry_p->ui8_data);
                  break;
                case OFPXMT_OFB_PBB_ISID:
                  of_sprintf(tmp_string," OFPXMT_OFB_PBB_ISID:= %d",action_entry_p->ui8_data);
                  break;
                case OFPXMT_OFB_TUNNEL_ID:
                  of_sprintf(tmp_string," OFPXMT_OFB_TUNNEL_ID:= %llx",action_entry_p->ui64_data);
                  break;
                case OFPXMT_OFB_VLAN_PCP:
                  of_sprintf(tmp_string," OFPXMT_OFB_VLAN_PCP:= %d",action_entry_p->ui8_data);
                  break;
                case OFPXMT_OFB_ARP_SPA:
                  of_sprintf(tmp_string," OFPXMT_OFB_ARP_SPA:= %x",action_entry_p->ui32_data);
                  break;
                case OFPXMT_OFB_ARP_TPA:
                  of_sprintf(tmp_string," OFPXMT_OFB_ARP_TPA:= %x",action_entry_p->ui32_data);
                  break;
                case OFPXMT_OFB_ARP_SHA:
                  of_sprintf(tmp_string," OFPXMT_OFB_ARP_SHA");
                  break;
                case OFPXMT_OFB_ARP_THA:
                  of_sprintf(tmp_string," OFPXMT_OFB_ARP_THA");
                  break;
                case OFPXMT_OFB_IP_DSCP:
                  of_sprintf(tmp_string," OFPXMT_OFB_IP_DSCP:= %d",action_entry_p->ui8_data);
                  break;
                case OFPXMT_OFB_IP_ECN:
                  of_sprintf(tmp_string," OFPXMT_OFB_IP_ECN:= %d",action_entry_p->ui8_data);
                  break;
                case OFPXMT_OFB_IPV6_SRC:
                  of_sprintf(tmp_string," OFPXMT_OFB_IPV6_SRC");
                  break;
                case OFPXMT_OFB_IPV6_DST:
                  of_sprintf(tmp_string," OFPXMT_OFB_IPV6_DST");
                  break;
                case OFPXMT_OFB_IPV6_FLABEL:
                  of_sprintf(tmp_string," OFPXMT_OFB_IPV6_FLABEL:= %d",action_entry_p->ui32_data);
                  break;
                case OFPXMT_OFB_ICMPV6_TYPE:
                  of_sprintf(tmp_string," OFPXMT_OFB_ICMPV6_TYPE:= %d",action_entry_p->ui8_data);
                  break;
                case OFPXMT_OFB_ICMPV6_CODE:
                  of_sprintf(tmp_string," OFPXMT_OFB_ICMPV6_CODE:= %d",action_entry_p->ui8_data);
                  break;
                case OFPXMT_OFB_IPV6_ND_TARGET:
                  of_sprintf(tmp_string," OFPXMT_OFB_IPV6_ND_TARGET");
                  break;
                case OFPXMT_OFB_IPV6_ND_SLL:
                  of_sprintf(tmp_string," OFPXMT_OFB_IPV6_ND_SLL");
                  break;
                case OFPXMT_OFB_IPV6_ND_TLL:
                  of_sprintf(tmp_string," OFPXMT_OFB_IPV6_ND_TLL");
                  break;
                case OFPXMT_OFB_IPV6_EXTHDR:
                  of_sprintf(tmp_string," OFPXMT_OFB_IPV6_EXTHDR= %d",action_entry_p->ui16_data);
                  break;
              }


            }
            break;

           case OFPAT_EXPERIMENTER:
             of_sprintf(string,"Type=EXPERIMENTER Vedor ID=%x SubType=%s", action_entry_p->vendorid,fslx_action_subtype[action_entry_p->sub_type]);
            break;

          default:
            of_sprintf(tmp_string,"%s", " - ");
            break;
        }
        strcat(string, tmp_string);
        if ( i == no_of_actions)
        {
          strcat(string, "\n");
        }
        FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_BINDSTATS_ACTIONINFO_ID,CM_DATA_TYPE_STRING, string);
        index++;
      }
    }
  }

  result_iv_pairs_p->count_ui = index;
  return OF_SUCCESS;
}


#endif /* OF_CM_SUPPORT */
