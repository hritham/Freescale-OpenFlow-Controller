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
 * File name: groupcbk.c
 * Author: G Atmaram <B38856@freescale.com>
 * Date:   03/13/2013
 * Description: 
 * 
 */

#ifdef OF_CM_SUPPORT

#include "cbcmninc.h"
#include "cmgutil.h"
#include "cntrucminc.h"
#include "of_utilities.h"
#include "of_multipart.h"
#include "of_group.h"
#include "fsl_ext.h"
#include "nicira_ext.h"

int32_t of_group_appl_ucmcbk_init (void);


void* of_group_starttrans(struct cm_array_of_iv_pairs   * key_iv_pairs,
    uint32_t command_id,
    struct cm_app_result ** result_pp);

int32_t  of_group_endtrans(void *config_trans_p,
    uint32_t command_id,
    struct cm_app_result ** result_pp);

int32_t of_group_addrec (void * config_trans_p,
    struct cm_array_of_iv_pairs * mand_iv_pairs,
    struct cm_array_of_iv_pairs * opt_iv_pairs,
    struct cm_app_result ** result_pp);

int32_t of_group_setrec (void * config_trans_p,
    struct cm_array_of_iv_pairs * mand_iv_pairs,
    struct cm_array_of_iv_pairs * opt_iv_pairs,
    struct cm_app_result ** result_pp);

int32_t of_group_delrec (void * config_trans_p,
    struct cm_array_of_iv_pairs * key_iv_pairs,
    struct cm_app_result ** result_pp);

int32_t of_group_getfirstnrecs (struct cm_array_of_iv_pairs * key_iv_pairs,
    uint32_t * count_p,
    struct cm_array_of_iv_pairs ** pArrayofIvPairArr);

int32_t of_group_getnextnrecs (struct cm_array_of_iv_pairs * key_iv_pairs,
    struct cm_array_of_iv_pairs * prev_rec_iv_pairs, uint32_t * count_p,
    struct cm_array_of_iv_pairs ** result_iv_pairs_pp);

int32_t of_group_getexactrec (struct cm_array_of_iv_pairs * key_iv_pairs,
    struct cm_array_of_iv_pairs ** result_iv_pairs_pp);

int32_t of_group_verifycfg (struct cm_array_of_iv_pairs * key_iv_pairs,
    uint32_t command_id, struct cm_app_result ** result_pp);

int32_t of_group_ucm_getparams (struct ofi_group_desc_info *group_info,
    struct cm_array_of_iv_pairs * result_iv_pairs_p);

int32_t of_group_verifycfg (struct cm_array_of_iv_pairs * key_iv_pairs,
    uint32_t command_id, struct cm_app_result ** result_pp);

int32_t of_group_ucm_setmandparams (struct cm_array_of_iv_pairs *
    mand_iv_pairs,
    struct dprm_datapath_general_info *datapath_info,
    struct ofi_group_desc_info *group_info,
    struct cm_app_result ** app_result_pp);

int32_t of_group_ucm_setoptparams (struct cm_array_of_iv_pairs *opt_iv_pairs,
                                   struct ofi_bucket * bucket_info,
                                   struct ofi_action * action_info,
                                   struct ofi_bucket_prop *bucket_prop,
                                   char *operation, uint8_t *afterbucketid,
                                   struct cm_app_result ** app_result_pp);

struct cm_dm_app_callbacks of_group_ucm_callbacks = 
{
  of_group_starttrans,
  of_group_addrec,
  of_group_setrec,
  of_group_delrec,
  of_group_endtrans,
  of_group_getfirstnrecs,
  of_group_getnextnrecs,
  of_group_getexactrec,
  of_group_verifycfg,
  NULL
};

uint32_t transId_g=0;
uint8_t bNodeExists_g=FALSE;
of_list_t group_trans_list_g={};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_group_appl_ucmcbk_init (void)
{
  int32_t return_value;

  CM_CBK_DEBUG_PRINT ("Entered");
  return_value=cm_register_app_callbacks (CM_ON_DIRECTOR_DATAPATH_GROUPS_GROUPDESC_APPL_ID,
      &of_group_ucm_callbacks);
  if(return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT("group application registration failed");
    return OF_FAILURE;
  }
  OF_LIST_INIT(group_trans_list_g,NULL);
  return OF_SUCCESS;
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_group_verifycfg (struct cm_array_of_iv_pairs * key_iv_pairs,
    uint32_t command_id, struct cm_app_result ** result_pp)
{
  struct cm_app_result *group_result = NULL;
  int32_t return_value = OF_FAILURE;

  CM_CBK_DEBUG_PRINT ("Entered");

  return_value = of_group_ucm_validatemandparams (key_iv_pairs, &group_result);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Validate Mandatory Parameters Failed");
    return OF_FAILURE;
  }
  *result_pp = group_result;
  return OF_SUCCESS;
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void* of_group_starttrans(struct cm_array_of_iv_pairs   * key_iv_pairs,
    uint32_t command_id,
    struct cm_app_result ** result_pp)
{
  struct group_trans *trans_rec=NULL;
  struct ofi_group_desc_info *group_info_p;
  struct dprm_datapath_general_info datapath_info={};
  struct cm_app_result *group_result = NULL;
  uint64_t datapath_handle;
  int32_t return_value;
  struct group_trans *trans_rec_entry_p=NULL;
  uchar8_t offset;

  CM_CBK_DEBUG_PRINT ("Entered");
  
  offset = OF_GROUP_TRANS_LISTNODE_OFFSET;

  group_info_p=(struct ofi_group_desc_info * ) calloc(1,sizeof(struct ofi_group_desc_info ));
  if ( group_info_p == NULL)
  {
    CM_CBK_DEBUG_PRINT ("memory allocation failed");
    return NULL;
  }

  if ((of_group_ucm_setmandparams (key_iv_pairs,&datapath_info, group_info_p, &group_result)) !=
      OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    return NULL;
  }

  return_value=dprm_get_datapath_handle(datapath_info.dpid, &datapath_handle);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Error: datapath does not exist with id %llx",datapath_info.dpid);
    fill_app_result_struct (&group_result, NULL, CM_GLU_GROUP_DATAPATH_DOESNOT_EXIST);
    *result_pp = group_result;
    return NULL;
  }

  OF_LIST_SCAN(group_trans_list_g, trans_rec_entry_p, struct group_trans *, offset) 
  {
    if (trans_rec_entry_p->group_id == group_info_p->group_id)
    {
       bNodeExists_g = TRUE;
       CM_CBK_DEBUG_PRINT ("node already exists in the list");
       /*Need to update Sub command id  */
       return (void *)trans_rec_entry_p;
    }
  }

  OF_GROUP_GENERATE_TRANS_ID(transId_g);
  trans_rec=(struct group_trans *) calloc(1,sizeof(struct group_trans));  
  trans_rec->datapath_handle=datapath_handle;
  trans_rec->trans_id=transId_g;
  trans_rec->command_id=command_id;
  trans_rec->group_id=group_info_p->group_id;
  trans_rec->group_type=group_info_p->group_type;
  CM_CBK_DEBUG_PRINT ("group type = %d",group_info_p->group_type);
  switch (command_id)
  {
    case CM_CMD_ADD_PARAMS:
      trans_rec->sub_command_id=ADD_GROUP;
#if 0
      //return_value =of_register_group(datapath_handle, group_info_p);
      if (return_value != OF_SUCCESS)
      {
        CM_CBK_DEBUG_PRINT ("Group Addition Failed");
        fill_app_result_struct (&group_result, NULL, CM_GLU_GROUP_ADD_FAILED);
        *result_pp =group_result;
        return NULL;
      }
#endif
      break;

    case CM_CMD_SET_PARAMS:
      trans_rec->sub_command_id=MOD_GROUP;
#if 0
      //return_value=of_group_unregister_buckets(datapath_handle, group_info_p->group_id);
      if (return_value != OF_SUCCESS)
      {
        CM_CBK_DEBUG_PRINT ("Group Modification Failed");
        fill_app_result_struct (&group_result, NULL, CM_GLU_GROUP_EDIT_FAILED);
        *result_pp =group_result;
        return NULL;
      }
#endif
      break;

    case CM_CMD_DEL_PARAMS:
      trans_rec->sub_command_id=DEL_GROUP;
      break;
  }
  bNodeExists_g = FALSE;
  OF_APPEND_NODE_TO_LIST(group_trans_list_g,trans_rec,offset);
  CM_CBK_DEBUG_PRINT ("node appended to list");
  return (void *)trans_rec;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int32_t of_group_addrec (void * config_trans_p,
    struct cm_array_of_iv_pairs * mand_iv_pairs,
    struct cm_array_of_iv_pairs * opt_iv_pairs,
    struct cm_app_result ** result_pp)
{
  struct cm_app_result *group_result = NULL;
  int32_t return_value = OF_FAILURE;
  struct ofi_group_desc_info group_info = { };
  struct dprm_datapath_general_info datapath_info={};
  uint64_t datapath_handle;
  struct ofi_bucket bucket_info;
  struct ofi_bucket_prop bucket_prop;
  struct ofi_action action_info;
  uint8_t afterbucketid;
  char operation[30];
  uchar8_t offset;
  offset = OF_GROUP_TRANS_LISTNODE_OFFSET;
  struct group_trans *trans_rec_entry_p=NULL;

  CM_CBK_DEBUG_PRINT ("Entered");
  of_memset (&group_info, 0, sizeof (struct ofi_group_desc_info));
  of_memset (&action_info, 0, sizeof (struct ofi_action));
  of_memset (&bucket_info, 0, sizeof (struct ofi_bucket));
  of_memset (&bucket_prop, 0, sizeof (struct ofi_bucket_prop));

  if ((of_group_ucm_setmandparams (mand_iv_pairs,&datapath_info, &group_info, &group_result)) !=
      OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    *result_pp=group_result;
    return OF_FAILURE;
  }

  return_value = of_group_ucm_setoptparams (opt_iv_pairs, &bucket_info, &action_info, 
                 &bucket_prop, operation, &afterbucketid, &group_result);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Optional Parameters Failed");
    *result_pp =group_result;
    return OF_FAILURE;
  }

  return_value=dprm_get_datapath_handle(datapath_info.dpid, &datapath_handle);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Error: datapath does not exist with id %llx",datapath_info.dpid);
    fill_app_result_struct (&group_result, NULL, CM_GLU_GROUP_DATAPATH_DOESNOT_EXIST);
    *result_pp = group_result;
    return OF_FAILURE;
  }
  OF_LIST_SCAN(group_trans_list_g, trans_rec_entry_p, struct group_trans *, offset) 
  {
    if (trans_rec_entry_p->group_id == group_info.group_id)
    {
       CM_CBK_DEBUG_PRINT ("node already exists in the list");
       trans_rec_entry_p->group_type = group_info.group_type;
       break;
    }
  }
  return_value = of_frame_and_send_add_group_info(datapath_handle, &group_info, 
                                 &bucket_info, &bucket_prop, &action_info);
  /*return_value = of_frame_and_send_add_group_info(datapath_handle, &group_info, 
                                                    NULL, NULL, NULL);*/
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Group Addition Failed");
    fill_app_result_struct (&group_result, NULL, CM_GLU_GROUP_ADD_FAILED);
    *result_pp =group_result;
    return OF_FAILURE;
  }
  return OF_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_group_setrec (void * config_trans_p,
    struct cm_array_of_iv_pairs * mand_iv_pairs,
    struct cm_array_of_iv_pairs * opt_iv_pairs,
    struct cm_app_result ** result_pp)
{
  struct cm_app_result *group_result = NULL;
  int32_t return_value = OF_FAILURE;
  struct ofi_group_desc_info group_info = { };
  struct dprm_datapath_general_info datapath_info={};
  struct ofi_bucket_prop bucket_prop;
  uint64_t datapath_handle;
  struct ofi_bucket bucket_info;
  struct ofi_action action_info;
  uint8_t afterbucketid;
  char operation[30];

  CM_CBK_DEBUG_PRINT ("Entered");
  of_memset (&group_info, 0, sizeof (struct ofi_group_desc_info));
  of_memset (&action_info, 0, sizeof (struct ofi_action));
  of_memset (&bucket_info, 0, sizeof (struct ofi_bucket));

  if ((of_group_ucm_setmandparams (mand_iv_pairs,&datapath_info, &group_info, &group_result)) !=
      OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    *result_pp=group_result;
    return OF_FAILURE;
  }

  return_value =of_group_ucm_setoptparams (opt_iv_pairs, &bucket_info, &action_info, 
                       &bucket_prop, operation, &afterbucketid, &group_result);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Optional Parameters Failed");
    *result_pp =group_result;
    return OF_FAILURE;
  }

  return_value=dprm_get_datapath_handle(datapath_info.dpid, &datapath_handle);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Error: datapath does not exist with id %llx",datapath_info.dpid);
    fill_app_result_struct (&group_result, NULL, CM_GLU_GROUP_DATAPATH_DOESNOT_EXIST);
    *result_pp = group_result;
    return OF_FAILURE;
  }

  /*if (strcmp(operation, "insert") == 0)
    return_value = of_frame_and_send_mod_group_bucket_info(datapath_handle, &group_info, 
                                          &bucket_info, &bucket_prop, &action_info);
  else if (strcmp(operation, "remove") == 0)
    return_value = of_frame_and_send_del_bucket_info(datapath_handle, &group_info, &bucket_info);
  else
    return OF_FAILURE;
  //return_value =of_update_group(datapath_handle, &group_info);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Group Addition Failed");
    fill_app_result_struct (&group_result, NULL, CM_GLU_GROUP_ADD_FAILED);
    *result_pp =group_result;
    return OF_FAILURE;
  }*/
  return OF_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_group_delrec (void * config_trans_p,
    struct cm_array_of_iv_pairs * key_iv_pairs,
    struct cm_app_result ** result_pp)
{
  struct cm_app_result *group_result = NULL;
  int32_t return_value = OF_FAILURE;
  struct ofi_group_desc_info group_info = { };
  struct dprm_datapath_general_info datapath_info={};
  uint64_t datapath_handle;
  struct group_trans *trans_rec=config_trans_p;
  struct group_trans *trans_rec_entry_p=NULL, *prev_trans_rec_p=NULL;
  uchar8_t offset;
  offset=  OF_GROUP_TRANS_LISTNODE_OFFSET;

  CM_CBK_DEBUG_PRINT ("Entered");
  of_memset (&group_info, 0, sizeof (struct ofi_group_desc_info));

  if ((of_group_ucm_setmandparams (key_iv_pairs,&datapath_info, &group_info, &group_result)) !=
      OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    fill_app_result_struct (&group_result, NULL, CM_GLU_SET_MAND_PARAM_FAILED);
    *result_pp =group_result;
    return OF_FAILURE;
  }

  return_value=dprm_get_datapath_handle(datapath_info.dpid, &datapath_handle);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Error: datapath does not exist with id %llx",datapath_info.dpid);
    fill_app_result_struct (&group_result, NULL, CM_GLU_GROUP_DATAPATH_DOESNOT_EXIST);
    *result_pp = group_result;
    return OF_FAILURE;
  }

  return_value = of_frame_and_send_delete_group(datapath_handle, group_info.group_id);
  
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Error: Delete group failed %d",  group_info.group_id);
    fill_app_result_struct (&group_result, NULL, CM_GLU_GROUP_DOESNOT_EXIST);
    *result_pp = group_result;
    return OF_FAILURE;
  }

  OF_LIST_SCAN(group_trans_list_g, trans_rec_entry_p, struct group_trans *, offset) 
  {
    if ((trans_rec_entry_p->trans_id == trans_rec->trans_id ) && 
                 (trans_rec_entry_p->group_id == group_info.group_id))
    {
      OF_REMOVE_NODE_FROM_LIST(  group_trans_list_g, trans_rec_entry_p,  prev_trans_rec_p, offset );
      CM_CBK_DEBUG_PRINT ("trans rec removed from list and freed");
      free(trans_rec_entry_p);
      break;
    }
    prev_trans_rec_p=trans_rec_entry_p;
  }
 
  return OF_SUCCESS;
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t  of_group_endtrans(void *config_trans_p,
    uint32_t command_id,
    struct cm_app_result ** result_pp)
{
  struct group_trans *trans_rec=config_trans_p;
  struct group_trans *trans_rec_entry_p=NULL, *prev_trans_rec_p=NULL;
  struct ofi_group_desc_info group_info={};
  struct cm_app_result *group_result = NULL;
  int32_t return_value = OF_FAILURE;
  uchar8_t offset;
 offset=  OF_GROUP_TRANS_LISTNODE_OFFSET;
  CM_CBK_DEBUG_PRINT ("Entered");


  CM_CBK_DEBUG_PRINT ("trans id %d command id %d sub cmd id %d group id %d",trans_rec->trans_id,trans_rec->command_id,
      trans_rec->sub_command_id,trans_rec->group_id);

  switch(command_id)
  {

    case CM_CMD_CONFIG_SESSION_REVOKE:
      CM_CBK_DEBUG_PRINT ("Revoke command");

      /*if (trans_rec->sub_command_id==ADD_GROUP)
      {
        //return_value=of_unregister_group(trans_rec->datapath_handle, trans_rec->group_id);
        if (return_value != OF_SUCCESS)
        {
          CM_CBK_DEBUG_PRINT ("Error: group does not exist with id %d",trans_rec->group_id);
          fill_app_result_struct (&group_result, NULL, CM_GLU_GROUP_DOESNOT_EXIST);
          *result_pp =group_result;
          return OF_FAILURE;
        }
      }*/
      OF_LIST_SCAN(group_trans_list_g, trans_rec_entry_p, struct group_trans *,offset)
      {
        if (trans_rec_entry_p->trans_id == trans_rec->trans_id )
        {
          if (bNodeExists_g == FALSE)
          {
            CM_CBK_DEBUG_PRINT ("trans rec removed from list and freed");
            OF_REMOVE_NODE_FROM_LIST(  group_trans_list_g, trans_rec_entry_p,  prev_trans_rec_p, offset);
            free(trans_rec_entry_p);
          }
          break;
        }
        prev_trans_rec_p=trans_rec_entry_p;
      }

      break;

    case CM_CMD_CONFIG_SESSION_COMMIT:
      CM_CBK_DEBUG_PRINT ("Commit command");
      if (trans_rec->sub_command_id==DEL_GROUP)
      {
      OF_LIST_SCAN(group_trans_list_g, trans_rec_entry_p, struct group_trans *, offset) 
      {
        if (trans_rec_entry_p->trans_id == trans_rec->trans_id )
        {
        OF_REMOVE_NODE_FROM_LIST(  group_trans_list_g, trans_rec_entry_p,  prev_trans_rec_p, offset );
          CM_CBK_DEBUG_PRINT ("trans rec removed from list and freed");
          free(trans_rec_entry_p);
          break;
        }
        prev_trans_rec_p=trans_rec_entry_p;
      }
      }
      break;

    default:
      CM_CBK_DEBUG_PRINT ("unknown command");

  }
  return OF_SUCCESS;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_group_getfirstnrecs(struct cm_array_of_iv_pairs * key_iv_pairs,
    uint32_t * count_p,
    struct cm_array_of_iv_pairs ** arr_ivpair_arr_pp)
{
  struct cm_array_of_iv_pairs *result_iv_pairs_p = NULL;
  int32_t return_value = OF_FAILURE;
  uint32_t uiRecCount = 0;
  struct cm_app_result *group_result = NULL;
  struct ofi_group_desc_info group_info={};
  struct dprm_datapath_general_info datapath_info={};
  uint64_t datapath_handle;

  CM_CBK_DEBUG_PRINT ("Entered");

  of_memset (&group_info, 0, sizeof (struct ofi_group_desc_info));
  if ((of_group_ucm_setmandparams (key_iv_pairs,&datapath_info, &group_info, &group_result)) !=
      OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    return OF_FAILURE;
  }

  return_value=dprm_get_datapath_handle(datapath_info.dpid, &datapath_handle);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Error: datapath does not exist with id %llx",datapath_info.dpid);
    return OF_FAILURE;
  }

  return_value=cntlr_send_group_desc_request(datapath_handle);
  if ( return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Group Desc failed");
    return OF_FAILURE;
  } 
  CM_CBK_DEBUG_PRINT ("Group Desc will be sent as a multipart response");
  return CM_CNTRL_MULTIPART_RESPONSE;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_group_getnextnrecs (struct cm_array_of_iv_pairs * key_iv_pairs,
    struct cm_array_of_iv_pairs *prev_rec_iv_pairs, uint32_t * count_p,
    struct cm_array_of_iv_pairs ** result_iv_pairs_pp)
{
  CM_CBK_DEBUG_PRINT ("no next records");
  *count_p=0;
  return OF_FAILURE;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_group_getexactrec (struct cm_array_of_iv_pairs * key_iv_pairs,
    struct cm_array_of_iv_pairs ** result_iv_pairs_pp)
{
  struct cm_array_of_iv_pairs *result_iv_pairs_p = NULL;
  struct cm_app_result *group_result = NULL;
  uint32_t ii = 0;
  int32_t return_value = OF_FAILURE;
  struct ofi_group_desc_info *group_entry_p;
  struct ofi_group_desc_info group_info={};
  struct dprm_datapath_general_info datapath_info={};
  uint64_t datapath_handle;
  struct group_trans *trans_rec_entry_p=NULL, *prev_trans_rec_p=NULL;
  uchar8_t offset;
  offset=  OF_GROUP_TRANS_LISTNODE_OFFSET;


  CM_CBK_DEBUG_PRINT ("Entered");

  of_memset (&group_info, 0, sizeof (struct ofi_group_desc_info));
  if ((of_group_ucm_setmandparams (key_iv_pairs,&datapath_info, &group_info, &group_result)) !=
      OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    return OF_FAILURE;
  }

      OF_LIST_SCAN(group_trans_list_g, trans_rec_entry_p, struct group_trans *, offset) 
      {
        if (trans_rec_entry_p != NULL)
        {
           CM_CBK_DEBUG_PRINT("trans_rec_entry_p->group_id = %d :: group_info.group_id=%d",trans_rec_entry_p->group_id, group_info.group_id);
           if (trans_rec_entry_p->group_id == group_info.group_id)
           {
              result_iv_pairs_p =
               (struct cm_array_of_iv_pairs *) of_calloc (1, sizeof (struct cm_array_of_iv_pairs));
             if (result_iv_pairs_p == NULL)
             {
               CM_CBK_DEBUG_PRINT ("Memory allocation failed for result_iv_pairs_p");
               return OF_FAILURE;
             }

             return_value=of_group_ucm_getparams (&group_info, &result_iv_pairs_p[0]);
             if ( return_value != OF_SUCCESS)
             {
               CM_CBK_DEBUG_PRINT ("of_group_ucm_getparams failed");
               return OF_FAILURE;
             } 

            *result_iv_pairs_pp = result_iv_pairs_p;
             return OF_SUCCESS;
           }
        }
      }
  return OF_FAILURE;

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_group_ucm_setmandparams (struct cm_array_of_iv_pairs *
    mand_iv_pairs,
    struct dprm_datapath_general_info *datapath_info,
    struct ofi_group_desc_info *group_info,
    struct cm_app_result ** app_result_pp)
{
  uint32_t mand_param_cnt;
  uint32_t group_id;
  uint64_t idpId;
  char *group_type;
  uint32_t group_type_len;

  CM_CBK_DEBUG_PRINT ("Entered");
  for (mand_param_cnt = 0; mand_param_cnt < mand_iv_pairs->count_ui;
      mand_param_cnt++)
  {
    switch (mand_iv_pairs->iv_pairs[mand_param_cnt].id_ui)
    {

      case CM_DM_DATAPATH_DATAPATHID_ID:
        idpId=charTo64bitNum((char *) mand_iv_pairs->iv_pairs[mand_param_cnt].value_p);
        CM_CBK_DEBUG_PRINT("dpid is: %llx",idpId);
        datapath_info->dpid=idpId;
        break;

      case CM_DM_GROUPDESC_GROUP_ID_ID:
        group_id=of_atoi((char *) mand_iv_pairs->iv_pairs[mand_param_cnt].value_p);
        group_info->group_id=group_id;
        CM_CBK_DEBUG_PRINT ("group id is %d", group_id);
        break;

      case CM_DM_GROUPDESC_GROUPTYPE_ID:
        group_type = ((char *) mand_iv_pairs->iv_pairs[mand_param_cnt].value_p);
	group_type_len = mand_iv_pairs->iv_pairs[mand_param_cnt].value_length;
	if(!strncmp(group_type,"all",group_type_len)){
 	       group_info->group_type=OFPGT_ALL;
	}
	else if(!strncmp(group_type,"select",group_type_len)){
 	       group_info->group_type=OFPGT_SELECT;
	}
	else if(!strncmp(group_type,"indirect",group_type_len)){
 	       group_info->group_type=OFPGT_INDIRECT;
	}
	else if(!strncmp(group_type,"ff",group_type_len)){
 	       group_info->group_type=OFPGT_FF;
	}
	else {
		CM_CBK_DEBUG_PRINT ("Group type not valid");
	}
        CM_CBK_DEBUG_PRINT ("group type is %s", group_type);
        break;

    }
  }
  CM_CBK_PRINT_IVPAIR_ARRAY (mand_iv_pairs);
  return OF_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_group_ucm_validatemandparams (struct cm_array_of_iv_pairs *
    mand_iv_pairs,
    struct cm_app_result ** app_result_pp)
{
  uint32_t mand_param_cnt;
  uint32_t group_id;
  char* type;
  uint32_t group_type_len;
  struct cm_app_result *group_result;

  CM_CBK_DEBUG_PRINT ("Entered");
  for (mand_param_cnt = 0; mand_param_cnt < mand_iv_pairs->count_ui;
      mand_param_cnt++)
  {
    switch (mand_iv_pairs->iv_pairs[mand_param_cnt].id_ui)
    {
      case CM_DM_GROUPDESC_GROUP_ID_ID:
        group_id=of_atoi((char *) mand_iv_pairs->iv_pairs[mand_param_cnt].value_p);
        CM_CBK_DEBUG_PRINT ("group id is %d", group_id);
        break;
      case CM_DM_GROUPDESC_GROUPTYPE_ID:
        type=((char *) mand_iv_pairs->iv_pairs[mand_param_cnt].value_p);
	group_type_len = mand_iv_pairs->iv_pairs[mand_param_cnt].value_length;
        CM_CBK_DEBUG_PRINT ("type is %s", type);
        if ((strncmp(type,"all",group_type_len) 
		&& strncmp(type,"select",group_type_len) 
		&& strncmp(type,"indirect",group_type_len) 
		&& strncmp(type,"ff",group_type_len)))
        {
          CM_CBK_DEBUG_PRINT ("group type is not valid");
          fill_app_result_struct (&group_result, NULL, CM_GLU_GROUP_TYPE_INVALID);
          *app_result_pp = group_result;
          return OF_FAILURE;
        }
        break;

    }
  }
  CM_CBK_PRINT_IVPAIR_ARRAY (mand_iv_pairs);
  return OF_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_group_ucm_setoptparams (struct cm_array_of_iv_pairs *opt_iv_pairs,
                                   struct ofi_bucket * bucket_info,
                                   struct ofi_action * action_info,
                                   struct ofi_bucket_prop *bucket_prop,
                                   char *operation, uint8_t *afterbucketid,
                                   struct cm_app_result ** app_result_pp)
{
  uint32_t uiOptParamCnt,max_len,ttl,ether_type,group_id,queue_id;
  uint8_t switch_type;
  char *data;
  char *action_type;
  uint32_t action_type_len;
  char *insert_position;
  uint32_t position_len;

  CM_CBK_DEBUG_PRINT ("Entered");

  for (uiOptParamCnt = 0; uiOptParamCnt < opt_iv_pairs->count_ui; uiOptParamCnt++)
  {
    switch (opt_iv_pairs->iv_pairs[uiOptParamCnt].id_ui)
    {
       case  CM_DM_GROUPDESC_BUCKETID_ID:
      	      bucket_info->bucket_id=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
      	      CM_CBK_DEBUG_PRINT ("bucket id %d", bucket_info->bucket_id);
       break;

       /*case  CM_DM_GROUPDESC_BUCKETOPERATION_ID:
      	      strcpy(operation, (char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
      	      CM_CBK_DEBUG_PRINT ("bucket operation %s", operation);
       break;*/

       case CM_DM_GROUPDESC_WEIGHT_ID:
              bucket_prop->weight=(uint16_t)of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
	     CM_CBK_DEBUG_PRINT ("weight  %d", bucket_prop->weight);
	break;

	case CM_DM_GROUPDESC_WATCH_PORT_ID:
		bucket_prop->watch_port=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		CM_CBK_DEBUG_PRINT ("watch_port  %d", bucket_prop->watch_port);
	break;
	case CM_DM_GROUPDESC_WATCH_GROUP_ID:
		bucket_prop->watch_group=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		CM_CBK_DEBUG_PRINT ("watch_group  %d", bucket_prop->watch_group);
	break;

	case CM_DM_GROUPDESC_PORT_NO_ID:
		data = (char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p;
		if ((*data == '0') &&
				((*(data+1) == 'x') || (*(data+1) == 'X')))
		{
			action_info->port_no = (uint32_t)atox_32(data+2);
		}
		else
		{
			action_info->port_no = (uint32_t)atoi(data);
		}
		CM_CBK_DEBUG_PRINT ("port_no  action_info->port_no %d", action_info->port_no );
	break;

	case CM_DM_GROUPDESC_MAX_LEN_ID:
		max_len=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		action_info->max_len=(uint16_t ) max_len;
		CM_CBK_DEBUG_PRINT ("max_len  %d action_info->max_len %d ", max_len,action_info->max_len);
	break;

	case CM_DM_GROUPDESC_TTL_ID:
		ttl=ucm_uint8_from_str_ptr((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		action_info->ttl= ttl;
		CM_CBK_DEBUG_PRINT ("ttl  %d action_info->ttl %d ", ttl,action_info->ttl);
		break;

	case CM_DM_GROUPDESC_ETHER_TYPE_ID:
		ether_type=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		action_info->ether_type=(uint16_t ) ether_type;
		CM_CBK_DEBUG_PRINT ("ether_type  %d action_info->ether_type %d ", ether_type,action_info->ether_type);
		break;

	case CM_DM_GROUPDESC_GROUPID_ID:
		group_id=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		action_info->group_id= group_id;
		CM_CBK_DEBUG_PRINT ("group_id  %d action_info->group_id %d ", group_id,action_info->group_id);
	break;

	case CM_DM_GROUPDESC_QUEUEID_ID:
		queue_id=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		action_info->queue_id= queue_id;
		CM_CBK_DEBUG_PRINT ("queue_id  %d action_info->queue_id %d ", queue_id,action_info->queue_id);
	break;

	case CM_DM_GROUPDESC_SETFIELDTYPE_ID:
		if(action_info->type != OFPAT_SET_FIELD)
		{
			CM_CBK_DEBUG_PRINT("Action type is not set_field ");
			return  OF_FAILURE;
		}
		if(of_group_set_action_setfield_type(action_info,
			(int8_t *)opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p,
		opt_iv_pairs->iv_pairs[uiOptParamCnt].value_length)!=OF_SUCCESS)
		{
			CM_CBK_DEBUG_PRINT("set action field type failed");
			return OF_FAILURE;
		}
	break;

	case CM_DM_GROUPDESC_SETFIELDVAL_ID:
		if(action_info->type != OFPAT_SET_FIELD)
		{
			CM_CBK_DEBUG_PRINT("Action type is not set_field");
			return OF_FAILURE;
		}
		if(of_group_set_action_setfieldtype_value(action_info,
		   (int8_t *)opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p,
		  opt_iv_pairs->iv_pairs[uiOptParamCnt].value_length)!=OF_SUCCESS)
		{
			CM_CBK_DEBUG_PRINT("set action field type value failed!.");
			return  OF_FAILURE;
		}
	break;
	case  CM_DM_GROUPDESC_ACTIONTYPE_ID:
		action_type=((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		action_type_len = (opt_iv_pairs->iv_pairs[uiOptParamCnt].value_length);
		of_group_set_action_type(action_info,action_type,action_type_len);

		//action_info->type=(uint16_t )action_type;
		CM_CBK_DEBUG_PRINT ("action type %d %s ",action_info->type,action_type);
         break;

      /*case CM_DM_GROUPDESC_INSERTPOSITION_ID:
           insert_position = ((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
           position_len = opt_iv_pairs->iv_pairs[uiOptParamCnt].value_length;
	   if(!strncmp(insert_position,"first",position_len)){
 	        //afterbucketid=OFPG_BUCKET_FIRST;
	   }
	   else if(!strncmp(insert_position,"last",position_len)){
 	       //afterbucketid=OFPG_BUCKET_LAST;
	   }
	   else if(!strncmp(insert_position,"afterbucket",position_len)){
 	       //group_info->group_type=OFPGT_INDIRECT;
	   }
	   else {
		CM_CBK_DEBUG_PRINT ("Insert position is not valid");
	   }
           CM_CBK_DEBUG_PRINT ("Insert position is %s", insert_position);
        break;

	case CM_DM_GROUPDESC_AFTERBUCKETID_ID:
            afterbucketid=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
	    CM_CBK_DEBUG_PRINT ("after bucket id = %d ", afterbucketid);
	break;*/

	default:
		CM_CBK_DEBUG_PRINT ("Invalid option");
                return OF_FAILURE;

    }
  }

  CM_CBK_PRINT_IVPAIR_ARRAY (opt_iv_pairs);
  return OF_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_group_ucm_getparams (struct ofi_group_desc_info *group_entry_p,
    struct cm_array_of_iv_pairs * result_iv_pairs_p)
{
  char string[1024];
  char tmp_string[1024];
  char buf_port[40];
  int32_t index = 0, jj, count_ui, iIndex;

  struct ofi_bucket *bucket_entry_p;
  struct ofi_bucket_prop *bucket_prop_entry_p;
  struct ofi_action *action_entry_p;
  int32_t watch_port, watch_group;
  int32_t no_of_actions, i;
  int16_t weight;
  char *group_actions[]={
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


  char *group_types[]={
    "OFPGT_ALL",
    "OFPGT_SELECT",
    "OFPGT_INDIRECT",
    "OFPGT_FF"
  };

  uchar8_t offset;


#define CM_GROUPDESC_CHILD_COUNT 20
  count_ui = CM_GROUPDESC_CHILD_COUNT;

  CM_CBK_DEBUG_PRINT ("Entered");
  result_iv_pairs_p->iv_pairs = (struct cm_iv_pair*)of_calloc(count_ui, sizeof(struct cm_iv_pair));
  if(!result_iv_pairs_p->iv_pairs)
  {
    CM_CBK_DEBUG_PRINT (" Memory allocation failed ");
    return OF_FAILURE;
  }

  of_memset(string, 0, sizeof(string));
  of_itoa (group_entry_p->group_id, string);
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_GROUPDESC_GROUP_ID_ID,CM_DATA_TYPE_STRING, string);
  index++;

  of_memset(string, 0, sizeof(string));
  of_sprintf(string,"%s",group_types[group_entry_p->group_type]);
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_GROUPDESC_GROUPTYPE_ID,CM_DATA_TYPE_STRING, string);
  index++;

  of_memset(string, 0, sizeof(string));
  of_sprintf(string,"%d\n",OF_LIST_COUNT(group_entry_p->bucket_list));
  FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_GROUPDESC_BUCKET_COUNT_ID,CM_DATA_TYPE_STRING, string);
  index++;

  if (OF_LIST_COUNT(group_entry_p->bucket_list) > 0 )
  {

    OF_LIST_SCAN(group_entry_p->bucket_list, bucket_entry_p, struct ofi_bucket *, OF_BUCKET_LISTNODE_OFFSET)
    {
       of_memset(string, 0, sizeof(string));
       of_itoa (bucket_entry_p->bucket_id, string);
       FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index], CM_DM_GROUPDESC_BUCKETID_ID, CM_DATA_TYPE_STRING, string);
       index++;

    if (OF_LIST_COUNT(bucket_entry_p->bucket_prop_list) > 0 )
    {
      OF_LIST_SCAN(bucket_entry_p->bucket_prop_list, bucket_prop_entry_p, struct ofi_bucket_prop *, OF_BUCKET_PROP_LISTNODE_OFFSET)
      {
        i++;
        of_memset(string, 0, sizeof(string));
        switch(group_entry_p->group_type)
        {
          case  OFPGT_ALL: /* All (multicast/broadcast) group.  */
            of_strcpy(string,"-");
            break;

          case OFPGT_SELECT: /* Select group. */
            weight=bucket_prop_entry_p->weight;
            of_sprintf(string,"weight=%d",weight);
            break;

          case OFPGT_INDIRECT: /* Indirect group. */
            OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"not supported");
            of_strcpy(string,"-");
            break;

          case OFPGT_FF: /* fast failover group */
            watch_port=bucket_prop_entry_p->watch_port;
            watch_group=bucket_prop_entry_p->watch_group;
            of_sprintf(string,"watch_port=%d watch_group=%d",watch_port, watch_group);
            break;

        }
        FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_GROUPDESC_BUCKETINFO_ID,CM_DATA_TYPE_STRING, string);
        index++;

      }
    }
    
    of_memset(string, 0, sizeof(string));
    of_sprintf(string,"%d\n",OF_LIST_COUNT(bucket_entry_p->action_list));
    FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_GROUPDESC_ACTION_COUNT_ID,CM_DATA_TYPE_STRING, string);
    index++;

    no_of_actions=OF_LIST_COUNT(bucket_entry_p->action_list);
    i=0;  
    if ( no_of_actions == 0 )
    {
      of_memset(string, 0, sizeof(string));
      of_sprintf(string,"%s", " - ");
      FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_GROUPDESC_ACTIONINFO_ID,CM_DATA_TYPE_STRING, string);
      index++;
    }
    else
    {
      OF_LIST_SCAN(bucket_entry_p->action_list, action_entry_p, struct ofi_action *, OF_ACTION_LISTNODE_OFFSET)
      {
        i++;
//
         of_memset(string, 0, sizeof(string));
        of_memset(tmp_string, 0, sizeof(tmp_string));
        of_memset(buf_port, 0, sizeof(buf_port));
        if(action_entry_p->type != OFPAT_EXPERIMENTER)
          of_sprintf(string,"Type=%s",group_actions[action_entry_p->type]);
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
                  of_sprintf(tmp_string," OFPXMT_OFB_ETH_DST  Value=%02x:%02x:%02x:%02x:%02x:%02x",action_entry_p->ui8_data_array[0],action_entry_p->ui8_data_array[1], action_entry_p->ui8_data_array[2],action_entry_p->ui8_data_array[3],action_entry_p->ui8_data_array[4],action_entry_p->ui8_data_array[5]);
                  break;
                case OFPXMT_OFB_ETH_SRC:
                  of_sprintf(tmp_string," OFPXMT_OFB_ETH_SRC  Value=%02x:%02x:%02x:%02x:%02x:%02x",action_entry_p->ui8_data_array[0],action_entry_p->ui8_data_array[1], action_entry_p->ui8_data_array[2],action_entry_p->ui8_data_array[3],action_entry_p->ui8_data_array[4],action_entry_p->ui8_data_array[5]);
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
            {
                    switch (action_entry_p->sub_type)
                    {
                                 case FSLXAT_ARP_RESPONSE:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_ARP_RESPONSE TargetMac= %llX", action_entry_p->vendorid,action_entry_p->ui64_data);
                                    }
                                    break;
                            case FSLXAT_IP_FRAGMENT:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_IP_FRAGMENT MTU= %d", action_entry_p->vendorid,action_entry_p->ui32_data);
                                    }
                                    break;
 case FSLXAT_IP_REASM:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_IP_REASM",action_entry_p->vendorid);
                                    }
                                    break;
                            case FSLXAT_DROP_PACKET:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_DROP_PACKET",action_entry_p->vendorid);
                                    }
                                    break;
                            case FSLXAT_SEND_ICMPV4_DEST_UNREACHABLE:
                                    {
                                            if(action_entry_p->ui8_data != FSLX_ICMPV4_DR_EC_DFBIT_SET)
                                                    of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_SEND_ICMPV4_DEST_UNREACHABLE Code= %d",action_entry_p->vendorid, action_entry_p->ui8_data);
                                            else
                                                    of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_SEND_ICMPV4_DEST_UNREACHABLE Code= %d NextHopMtu %d", action_entry_p->vendorid,action_entry_p->ui8_data,action_entry_p->ui16_data);
                                    }
                                    break;
                            case FSLXAT_SEND_ICMPV4_TIME_EXCEED:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_SEND_ICMPV4_TIME_EXCEED Code %d",action_entry_p->vendorid, action_entry_p->ui8_data);
                                    }
                                    break;
case FSLXAT_SET_PHY_PORT_FIELD_CNTXT:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_SET_PHY_PORT_FIELD_CNTXT PhyInPort %d",action_entry_p->vendorid, action_entry_p->ui32_data);
                                    }
                                    break;
                            case FSLXAT_SET_META_DATA_FIELD_CNTXT:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_SET_META_DATA_FIELD_CNTXT MetaData= %llX",action_entry_p->vendorid, action_entry_p->ui64_data);
                                    }
                                    break;
                            case FSLXAT_SET_TUNNEL_ID_FIELD_CNTXT:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_SET_TUNNEL_ID_FIELD_CNTXT Tunnel_id= %llX", action_entry_p->vendorid,action_entry_p->ui64_data);
                                    }
 break;
                            case FSLXAT_START_TABLE:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_START_TABLE Table_id= %d",action_entry_p->vendorid, action_entry_p->ui8_data);
                                    }
                                    break;
                            case FSLXAT_WRITE_METADATA_FROM_PKT:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_WRITE_METADATA_FROM_PKT Field_id= %d Mask= %llX", action_entry_p->vendorid,action_entry_p->ui32_data,action_entry_p->ui64_data);
                                    }
                                    break;
                            case FSLXAT_WRITE_META_DATA:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_WRITE_META_DATA MetaData= %llX",action_entry_p->vendorid, action_entry_p->ui64_data);
                                    }
                                    break;
 case FSLXAT_DETACH_BIND_OBJ:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_DETACH_BIND_OBJ BindObjId= %d", action_entry_p->vendorid,action_entry_p->ui32_data);
                                            break;
                                    }
                            case FSLXAT_SEND_APP_STATE_INFO:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_SEND_APP_STATE_INFO Type= %d Method= %d Count= %d", action_entry_p->vendorid,action_entry_p->ui8_data,action_entry_p->ui16_data,action_entry_p->ui64_data);
                                            break;
                                    }

                            case FSLXAT_BIND_OBJECT_APPLY_ACTIONS:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_BIND_OBJECT_APPLY_ACTIONS BindObjId= %d",action_entry_p->vendorid, action_entry_p->ui32_data);
                                            break;
                                    }

                            case FSLXAT_SET_APP_STATE_INFO:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_SET_APP_STATE_INFO",action_entry_p->vendorid);
                                            /* struct fslx_action_set_app_state_info */
                                            break;
     }

                            case FSLXAT_CREATE_SA_CNTXT:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_CREATE_SA_CNTXT",action_entry_p->vendorid);
                                            /* struct fslx_create_ipsec_sa */
                                            break;
                                    }
                            case FSLXAT_DELETE_SA_CNTXT:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_DELETE_SA_CNTXT",action_entry_p->vendorid);
                                            /* struct fslx_delete_ipsec_sa */
                                            break;
                                    }
 case FSLXAT_IPSEC_ENCAP:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_IPSEC_ENCAP",action_entry_p->vendorid);
                                            /* struct fslx_ipsec_encap */
                                            break;
                                    }

                            case FSLXAT_IPSEC_DECAP:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_IPSEC_DECAP",action_entry_p->vendorid);
                                            /* struct fslx_ipsec_decap */
                                            break;
                                    }

                            case FSLXAT_POP_NATT_UDP_HEADER:
                                    {
                                            of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=FSLXAT_POP_NATT_UDP_HEADER",action_entry_p->vendorid);
                                            break;
                                    }

  case NXAST_REG_LOAD:
                                   // of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType= NXAST_REG_LOAD", action_entry_p->vendorid);
                                         of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType= NXAST_REG_LOAD-%d NBITS= %d OFFSET= %d  VALUE= %lld", action_entry_p->vendorid,action_entry_p->ui32_data,action_entry_p->ui8_data,action_entry_p->ui16_data,action_entry_p->ui64_data);

                                    break;
                            case NXAST_COPY_FIELD:
                                    of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=NXAST_COPY_FIELD", action_entry_p->vendorid);
                                    break;
                            default:
                                    of_sprintf(tmp_string,"Type=EXPERIMENTER Vendor=%x SubType=Unknown", action_entry_p->vendorid);
                                    break;
                    }

             }
            break;

          default:
            of_sprintf(tmp_string,"%s", " - ");
            break;
        }

//
        strcat(string, tmp_string);
        if ( i == no_of_actions)
        {
          strcat(string, "\n");
        }
        FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_GROUPDESC_ACTIONINFO_ID,CM_DATA_TYPE_STRING, string);
        index++;
      }
    }
   }  
  }
  result_iv_pairs_p->count_ui = index;

  CM_CBK_PRINT_IVPAIR_ARRAY (result_iv_pairs_p);
  return OF_SUCCESS;
}

int32_t group_set_action_setfield_type(struct ofi_action *action_info,
		    char *setfield_type, uint32_t setfield_len) 
{                                       
	   OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ALL,"of_group_set_action_setfield_type Entered & set_field_type:%s ",setfield_type);                                     

	   if (!strncmp(setfield_type, "srcIpV4Addr", setfield_len))
	   {                             
		   action_info->setfield_type = OFPXMT_OFB_IPV4_SRC;
	   }                             
	   else if (!strncmp(setfield_type, "dstIpV4Addr", setfield_len))
	   {                                     
		   action_info->setfield_type = OFPXMT_OFB_IPV4_DST;
	   }                                             
	   else if (!strncmp(setfield_type, "udpSrcPort", setfield_len))
	   {                                     
		   action_info->setfield_type = OFPXMT_OFB_UDP_SRC;
	   }                                       
	   else if (!strncmp(setfield_type, "udpDstPort", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_UDP_DST;
	   }
	   else if (!strncmp(setfield_type, "tcpSrcPort", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_TCP_SRC;
	   }
	   else if (!strncmp(setfield_type, "tcpDstPort", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_TCP_DST;
	   }
	   else if (!strncmp(setfield_type, "sctpSrcPort", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_SCTP_SRC;
	   }
	   else if (!strncmp(setfield_type, "sctpDstPort", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_SCTP_DST;
	   }
	   else if (!strncmp(setfield_type, "protocol", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_IP_PROTO;
	   }
	   else if (!strncmp(setfield_type, "ethType", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_ETH_TYPE;
	   }
	   else if (!strncmp(setfield_type, "srcMacAddr", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_ETH_SRC;
	   }
	   else if (!strncmp(setfield_type, "dstMacAddr", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_ETH_DST;
	   }
	   else if (!strncmp(setfield_type, "icmpType", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_ICMPV4_TYPE;
	   }
	   else if (!strncmp(setfield_type, "icmpCode", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_ICMPV4_CODE;
	   }
	   else if (!strncmp(setfield_type, "inPort", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_IN_PORT;
	   }
	   else if (!strncmp(setfield_type, "inPhyPort", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_IN_PHY_PORT;
	   }
	   else if (!strncmp(setfield_type, "tableMetaData", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_METADATA;
	   }
	   else if (!strncmp(setfield_type, "mplsLabel", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_MPLS_LABEL;
	   }
	   else if (!strncmp(setfield_type, "mplsTC", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_MPLS_TC;
	   }
	   else if (!strncmp(setfield_type, "mplsBos", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_MPLS_BOS;
	   }
	   else if (!strncmp(setfield_type, "pbbIsid", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_PBB_ISID;
	   }
	   else if (!strncmp(setfield_type, "tunnelId", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_TUNNEL_ID;
	   }
	   else if (!strncmp(setfield_type, "VlanId", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_VLAN_VID;
	   }
	   else if (!strncmp(setfield_type, "VlanPriority", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_VLAN_PCP;
	   }
	   else if (!strncmp(setfield_type, "arpSrcIpv4Addr", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_ARP_SPA;
	   }
	   else if (!strncmp(setfield_type, "arpDstIpv4Addr", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_ARP_TPA;
	   }
	   else if (!strncmp(setfield_type, "arpSrcMacAddr", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_ARP_SHA;
	   }
	   else if (!strncmp(setfield_type, "arpDstMacAddr", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_ARP_THA;
	   }
	   else if (!strncmp(setfield_type, "arpOpcode", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_ARP_OP;
	   }
	   else if (!strncmp(setfield_type, "IpDiffServCodePointBits", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_IP_DSCP;
	   }
	   else if (!strncmp(setfield_type, "IpECNBits", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_IP_ECN;
	   }
	   else if (!strncmp(setfield_type, "srcIpv6Addr", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_IPV6_SRC;
	   }
	   else if (!strncmp(setfield_type, "dstIpv6Addr", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_IPV6_DST;
	   }
	   else if (!strncmp(setfield_type, "icmpv6Type", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_ICMPV6_TYPE;
	   }
	   else if (!strncmp(setfield_type, "icmpv6Code", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_ICMPV6_CODE;
	   }
	   else if (!strncmp(setfield_type, "ipv6NDTarget", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_IPV6_ND_TARGET;
	   }
	   else if (!strncmp(setfield_type, "ipv6NDSrcLinkLayerAddr", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_IPV6_ND_SLL;
	   }
	   else if (!strncmp(setfield_type, "ipv6NDTargetLinkLayerAddr", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_IPV6_ND_TLL;
	   }
	   else if (!strncmp(setfield_type, "Ipv6flowlabel", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_IPV6_FLABEL;
	   }
	   else if (!strncmp(setfield_type, "ipv6ExtnHdrPseudofield", setfield_len))
	   {
		   action_info->setfield_type = OFPXMT_OFB_IPV6_EXTHDR;
	   }
	   else
	   {
		   OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR, "Invalide set field type ....");
		   return OF_FAILURE;
	   }
	   return OF_SUCCESS;
}

int32_t group_set_action_setfieldtype_value(struct ofi_action *action_info,
		char *data, uint32_t data_len)
{
	int32_t retval = OF_SUCCESS;
	uint32_t field_type = action_info->setfield_type;
	OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ALL," of_flow_set_action_setfieldtype_value Entered & setfieldtype_value:%s",data);

	do
	{
		switch (field_type)
		{
			case OFPXMT_OFB_IN_PORT: /* Switch input port. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG," OFPXMT_OFB_IN_PORT:");
					action_info->ui32_data = (uint32_t)atoi(data);
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "INPORT=%d",action_info->ui32_data);
					break;
				}
			case OFPXMT_OFB_IN_PHY_PORT: /* Switch physical input port. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_IN_PHY_PORT:");
					action_info->ui32_data = (uint32_t)atoi(data);
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "INPhy-port=%d",action_info->ui32_data);

					break;
				}
			case OFPXMT_OFB_METADATA: /* Metadata passed between tables. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_METADATA:");
					if ((*data == '0') && ((*(data+1) == 'x') || (*(data+1) == 'X')))
					{
						action_info->ui64_data = atox_64(data+2);
					}
					else
					{
						action_info->ui64_data = (uint64_t)atoll(data);
					}
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "metadata=%llx",action_info->ui64_data);
					break;
				}
			case OFPXMT_OFB_ETH_DST: /* Ethernet destination address. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_ETH_DST:");
					if ((*data == '0') && ((*(data+1) == 'x') || (*(data+1) == 'X')))
					{
						of_flow_match_atox_mac_addr(data+2, action_info->ui8_data_array);
					}
					else
					{
						of_flow_match_atox_mac_addr(data, action_info->ui8_data_array);
					}
					break;
				}
			case OFPXMT_OFB_ETH_SRC: /* Ethernet source address. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_ETH_SRC: ");
					if ((*data == '0') &&
							((*(data+1) == 'x') || (*(data+1) == 'X')))
					{
						of_flow_match_atox_mac_addr(data+2, action_info->ui8_data_array);
					}
					else
					{
						of_flow_match_atox_mac_addr(data, action_info->ui8_data_array);
					}
					break;
				}
			case OFPXMT_OFB_ETH_TYPE: /* Ethernet frame type. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_ETH_TYPE: ");
					if ((*data == '0') &&
							((*(data+1) == 'x') || (*(data+1) == 'X')))
					{
						action_info->ui16_data = (uint16_t)atox_32(data+2);
					}
					else
					{
						action_info->ui16_data = (uint16_t)atoi(data);
					}
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"ethtype=%x",action_info->ui16_data);
					break;
				}
			case OFPXMT_OFB_VLAN_VID: /* VLAN id. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_VLAN_VID: ");
					action_info->ui16_data = (uint16_t)atoi(data);
					break;
				}
			case OFPXMT_OFB_IP_PROTO: /* IP protocol. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_IP_PROTO: ");
					action_info->ui8_data = (uint8_t)atoi(data);
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "protocol=%d",action_info->ui8_data);
					break;
				}
			case OFPXMT_OFB_IPV4_SRC: /* IPv4 source address. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_IPV4_SRC: ");
					action_info->ui32_data = (uint32_t)inet_network(data);
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"src ip=%x",action_info->ui32_data);
					break;
				}
			case OFPXMT_OFB_IPV4_DST: /* IPv4 destination address. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_IPV4_DST: ");
					action_info->ui32_data = (uint32_t)inet_network(data);
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "dest-ip=%x",action_info->ui32_data);
					break;
				}
			case OFPXMT_OFB_TCP_SRC: /* TCP source port. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_TCP_SRC: ");
					action_info->ui16_data = (uint16_t)atoi(data);
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "tcp srcport=%d",action_info->ui16_data);
					break;
				}
			case OFPXMT_OFB_TCP_DST: /* TCP destination port. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_TCP_DST: ");
					action_info->ui16_data = (uint16_t)atoi(data);
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "tcp destport=%d",action_info->ui16_data);
					break;
				}
			case OFPXMT_OFB_UDP_SRC: /* UDP source port. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_UDP_SRC: ");
					action_info->ui16_data = (uint16_t)atoi(data);

					break;
				}
			case OFPXMT_OFB_UDP_DST: /* UDP destination port. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_UDP_DST: ");
					action_info->ui16_data = (uint16_t)atoi(data);

					break;
				}
			case OFPXMT_OFB_SCTP_SRC: /* SCTP source port. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_SCTP_SRC: ");
					action_info->ui16_data = (uint16_t)atoi(data);

					break;
				}
			case OFPXMT_OFB_SCTP_DST: /* SCTP destination port. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_SCTP_DST: ");
					action_info->ui16_data = (uint16_t)atoi(data);

					break;
				}
			case OFPXMT_OFB_ICMPV4_TYPE: /* ICMP type. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_ICMPV4_TYPE:");
					action_info->ui8_data = (uint8_t)atoi(data);

					break;
				}
			case OFPXMT_OFB_ICMPV4_CODE: /* ICMP code. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_ICMPV4_CODE: ");
					action_info->ui8_data = (uint8_t)atoi(data);
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "icmp-code=%d",action_info->ui8_data);
					break;
				}
			case OFPXMT_OFB_ARP_OP: /* ARP opcode. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_ARP_OP: ");
					action_info->ui16_data = (uint16_t)atoi(data);
					break;
				}
			case OFPXMT_OFB_MPLS_LABEL: /* MPLS label. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_MPLS_LABEL:");
					action_info->ui32_data = (uint32_t)atoi(data);
					break;
				}
			case OFPXMT_OFB_MPLS_TC: /* MPLS TC. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_MPLS_TC:");
					action_info->ui8_data = (uint8_t)atoi(data);
					break;
				}
			case OFPXMT_OFB_MPLS_BOS: /* MPLS BoS bit. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_MPLS_BOS:");
					action_info->ui8_data = (uint8_t)atoi(data);
					break;
				}
			case OFPXMT_OFB_PBB_ISID: /* PBB I-SID. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_PBB_ISID:");
					action_info->ui8_data = (uint8_t)atoi(data);
					break;
				}
			case OFPXMT_OFB_TUNNEL_ID: /* Logical Port Metadata. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_TUNNEL_ID:");
					action_info->ui64_data = (uint64_t)atoll(data);
					break;
				}
			case OFPXMT_OFB_VLAN_PCP:
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_VLAN_PCP");
					action_info->ui8_data = (uint8_t)atoi(data);
					break;
				}
			case OFPXMT_OFB_ARP_SPA: /* IPv4 source address. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_ARP_SPA: ");
					action_info->ui32_data = (uint32_t)inet_network(data);
					break;
				}
			case OFPXMT_OFB_ARP_TPA: /* IPv4 source address. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_ARP_TPA: ");
					action_info->ui32_data = (uint32_t)inet_network(data);
					break;
				}
			case OFPXMT_OFB_ARP_SHA:
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "OFPXMT_OFB_ARP_SHA: ");
					if ((*data == '0') &&
							((*(data+1) == 'x') || (*(data+1) == 'X')))
					{
						of_flow_match_atox_mac_addr(data+2, action_info->ui8_data_array);
					}
					else
					{
						of_flow_match_atox_mac_addr(data, action_info->ui8_data_array);
					}
					break;
				}
			case OFPXMT_OFB_ARP_THA:
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "OFPXMT_OFB_ARP_THA: ");
					if ((*data == '0') &&
							((*(data+1) == 'x') || (*(data+1) == 'X')))
					{
						of_flow_match_atox_mac_addr(data+2, action_info->ui8_data_array);
					}
					else
					{
						of_flow_match_atox_mac_addr(data, action_info->ui8_data_array);
					}
					break;
				}
			case OFPXMT_OFB_IP_DSCP:
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "OFPXMT_OFB_IP_DSCP: ");
					action_info->ui8_data = (uint8_t)atoi(data);
					break;
				}
			case OFPXMT_OFB_IP_ECN:
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "OFPXMT_OFB_IP_ECN: ");
					action_info->ui8_data = (uint8_t)atoi(data);
					break;
				}
			case OFPXMT_OFB_IPV6_SRC:
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "OFPXMT_OFB_IPV6_SRC");
					if(inet_pton(AF_INET6, data, &(action_info->ipv6_addr.ipv6Addr32)) != 1)
					{
						OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "Invalid Ipv6 src address!. ");
						return OF_FAILURE;
					}
					break;
				}
			case OFPXMT_OFB_IPV6_DST:
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "OFPXMT_OFB_IPV6_DST");
					if(inet_pton(AF_INET6, data, &(action_info->ipv6_addr.ipv6Addr32)) != 1)
					{
						OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "Invalid Ipv6 dst address!. ");
						return OF_FAILURE;
					}
					break;
				}
			case OFPXMT_OFB_IPV6_FLABEL:
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "OFPXMT_OFB_IPV6_FLABEL ");
					if ((*data == '0') &&
							((*(data+1) == 'x') || (*(data+1) == 'X')))
					{
						action_info->ui32_data = (uint32_t)atox_32(data+2);
					}
					else
					{
						action_info->ui32_data = (uint32_t)atoi(data);
					}
					break;
				}
			case OFPXMT_OFB_ICMPV6_TYPE: /* ICMP type. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_ICMPV6_TYPE:");
					if ((*data == '0') &&
							((*(data+1) == 'x') || (*(data+1) == 'X')))
					{
						action_info->ui8_data = (uint8_t)atox_32(data+2);
					}
					else
					{
						action_info->ui8_data = (uint8_t)atoi(data);
					}
					break;
				}
			case OFPXMT_OFB_ICMPV6_CODE: /* ICMP code. */
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPXMT_OFB_ICMPV6_CODE: ");
					if ((*data == '0') &&
							((*(data+1) == 'x') || (*(data+1) == 'X')))
					{
						action_info->ui8_data = (uint8_t)atox_32(data+2);
					}
					else
					{
						action_info->ui8_data = (uint8_t)atoi(data);
					}
					action_info->ui8_data = (uint8_t)atoi(data);
					break;
				}
			case OFPXMT_OFB_IPV6_ND_TARGET:
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "OFPXMT_OFB_IPV6_ND_TARGET: ");

					if(inet_pton(AF_INET6, data, &(action_info->ipv6_addr.ipv6Addr32)) != 1)
					{
						OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "Invalid Ipv6  address!. ");
					}
					break;
				}
			case OFPXMT_OFB_IPV6_ND_SLL: /* The source link-layer address option in an IPv6Neighbor Discovery message.*/                    
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "OFPXMT_OFB_IPV6_ND_SLL:");
					if ((*data == '0') && ((*(data+1) == 'x') || (*(data+1) == 'X')))
					{
						of_flow_match_atox_mac_addr(data+2, action_info->ui8_data_array);
					}
					else
					{
						of_flow_match_atox_mac_addr(data, action_info->ui8_data_array);
					}
					break;
				}
			case OFPXMT_OFB_IPV6_ND_TLL: /* The target link-layer address option in an IPv6Neighbor Discovery message.*/
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG," OFPXMT_OFB_IPV6_ND_TLL: ");
					if ((*data == '0') &&
							((*(data+1) == 'x') || (*(data+1) == 'X')))
					{
						of_flow_match_atox_mac_addr(data+2, action_info->ui8_data_array);
					}
					else
					{
						of_flow_match_atox_mac_addr(data, action_info->ui8_data_array);
					}
					break;
				}
			case OFPXMT_OFB_IPV6_EXTHDR:
				{
					OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG, "OFPXMT_OFB_IPV6_EXTHDR: ");
					if ((*data == '0') &&
							((*(data+1) == 'x') || (*(data+1) == 'X')))
					{
						action_info->ui16_data = (uint16_t)atox_32(data+2);
					}
					else
					{
						action_info->ui16_data = (uint16_t)atoi(data);
					}
					break;
				}
		}
	}
	while(0);

	return retval;
}

#endif /*OF_CM_SUPPORT */
