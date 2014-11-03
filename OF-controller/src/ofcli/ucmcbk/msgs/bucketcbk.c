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
 * File name: bucketcbk.c
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

int32_t of_bucket_appl_ucmcbk_init (void);


void* of_bucket_starttrans(struct cm_array_of_iv_pairs   * key_iv_pairs,
		uint32_t command_id,
		struct cm_app_result ** result_pp);

int32_t  of_bucket_endtrans(void *config_trans_p,
		uint32_t command_id,
		struct cm_app_result ** result_pp);

int32_t of_bucket_addrec (void * config_trans_p,
		struct cm_array_of_iv_pairs * mand_iv_pairs,
		struct cm_array_of_iv_pairs * opt_iv_pairs,
		struct cm_app_result ** result_pp);

int32_t of_bucket_setrec (void * config_trans_p,
		struct cm_array_of_iv_pairs * mand_iv_pairs,
		struct cm_array_of_iv_pairs * opt_iv_pairs,
		struct cm_app_result ** result_pp);

int32_t of_bucket_delrec (void * config_trans_p,
		struct cm_array_of_iv_pairs * key_iv_pairs,
		struct cm_app_result ** result_pp);

int32_t of_bucket_getfirstnrecs (struct cm_array_of_iv_pairs * key_iv_pairs,
		uint32_t * pCount,
		struct cm_array_of_iv_pairs ** pArrayofIvPairArr);

int32_t of_bucket_getnextnrecs (struct cm_array_of_iv_pairs * key_iv_pairs,
		struct cm_array_of_iv_pairs * pPrevRecordKey, uint32_t * pCount,
		struct cm_array_of_iv_pairs ** pNextNRecordData_p);

int32_t of_bucket_getexactrec (struct cm_array_of_iv_pairs * key_iv_pairs,
		struct cm_array_of_iv_pairs ** pIvPairArr);

int32_t of_bucket_verifycfg (struct cm_array_of_iv_pairs * key_iv_pairs,
		uint32_t command_id, struct cm_app_result ** result_pp);

 int32_t of_bucket_ucm_getparams (struct ofi_bucket *group_info,
		struct cm_array_of_iv_pairs * result_iv_pairs_p);

 int32_t of_bucket_ucm_setmandparams (struct cm_array_of_iv_pairs *mand_iv_pairs,
		struct  ofi_group_desc_info *group_info,
		struct ofi_bucket *bucket_info, char *operation, uint32_t *position,
		struct cm_app_result ** app_result_pp);

int32_t of_bucket_ucm_setoptparams (struct cm_array_of_iv_pairs *opt_iv_pairs,
                                   struct ofi_bucket_prop * bucket_prop,
                                   struct ofi_action * action_info,
                                   char *operation, uint32_t *afterbucketid,
                                   struct cm_app_result ** app_result_pp);

struct cm_dm_app_callbacks of_bucket_ucm_callbacks = 
{
	of_bucket_starttrans,
	of_bucket_addrec,
	of_bucket_setrec,
	of_bucket_delrec,
	of_bucket_endtrans,
	of_bucket_getfirstnrecs,
	of_bucket_getnextnrecs,
	of_bucket_getexactrec,
	of_bucket_verifycfg,
	NULL
};

extern of_list_t group_trans_list_g;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_bucket_appl_ucmcbk_init (void)
{
	int32_t return_value;

	return_value=cm_register_app_callbacks (CM_ON_DIRECTOR_DATAPATH_GROUPS_GROUPDESC_BUCKET_APPL_ID,
			&of_bucket_ucm_callbacks);
	if(return_value != OF_SUCCESS)
	{
		cntrlrucm_debugmsg("bucket application CBK registration failed");
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
int32_t of_bucket_verifycfg (struct cm_array_of_iv_pairs * key_iv_pairs,
		uint32_t command_id, struct cm_app_result ** result_pp)
{
	struct cm_app_result *of_bucket_result = NULL;
	int32_t return_value = OF_FAILURE;

	CM_CBK_DEBUG_PRINT ("Entered");

	return_value = of_bucket_ucm_validatemandparams (key_iv_pairs, &of_bucket_result);
	if (return_value != OF_SUCCESS)
	{
		CM_CBK_DEBUG_PRINT ("Validate Mandatory Parameters Failed");
		return OF_FAILURE;
	}
	*result_pp = of_bucket_result;
	return OF_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void* of_bucket_starttrans(struct cm_array_of_iv_pairs   * key_iv_pairs,
		uint32_t command_id,
		struct cm_app_result ** result_pp)
{
	struct group_trans *trans_rec;
	struct ofi_group_desc_info group_info={};
	struct ofi_bucket *bucket_info=NULL;
	struct cm_app_result *of_bucket_result = NULL;
	int32_t return_value;
        uint64_t  datapath_handle;
        struct dprm_datapath_general_info datapath_info={};
        char operation[30];
        uint8_t position;

        uchar8_t offset;
        offset = OF_GROUP_TRANS_LISTNODE_OFFSET;

	CM_CBK_DEBUG_PRINT ("Entered");

	bucket_info=(struct ofi_bucket *) calloc (1, sizeof(struct ofi_bucket));
	if( bucket_info == NULL )
	{
		CM_CBK_DEBUG_PRINT ("memory allocation failed");
		return NULL;
	}

	if ((of_bucket_ucm_setmandparams (key_iv_pairs,&group_info, bucket_info, operation, &position, &of_bucket_result)) !=
			OF_SUCCESS)
	{
		CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
		return NULL;
	}

	OF_LIST_SCAN(group_trans_list_g, trans_rec, struct group_trans *,offset )
	{
		if(trans_rec->group_id == group_info.group_id)
		{
			CM_CBK_DEBUG_PRINT ("transaction found");
			break;
		}

	}

#if 0
        return_value=dprm_get_datapath_handle(datapath_info.dpid, &datapath_handle);
        /*return_value = of_inform_dp_2_add_port_4_multicast_traffic(trans_rec->datapath_handle,
                                                       group_info.group_id,1,1);*/
                                                       //bucket_info->bucket_id, bucket_info->watch_port);
	//return_value =of_register_bucket_to_group(trans_rec->datapath_handle,bucket_info,group_info.group_id);
	if (return_value != OF_SUCCESS)
	{
		CM_CBK_DEBUG_PRINT ("Bucket Addition to group %d Failed", group_info.group_id);
		fill_app_result_struct (&of_bucket_result, NULL, CM_GLU_BUCKET_ADD_FAILED);
		*result_pp =of_bucket_result;
		return NULL;
	}
#endif
	CM_CBK_DEBUG_PRINT ("transaction pointer %p",trans_rec);
	return (void *)trans_rec;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

int32_t of_bucket_addrec (void * config_trans_p,
		struct cm_array_of_iv_pairs * mand_iv_pairs,
		struct cm_array_of_iv_pairs * opt_iv_pairs,
		struct cm_app_result ** result_pp)
{
	struct cm_app_result *of_bucket_result = NULL;
	int32_t return_value = OF_FAILURE;
	struct ofi_bucket bucket_info = { };
	struct ofi_group_desc_info group_info = { };
	struct group_trans *trans_rec=(struct group_trans *)config_trans_p;
        struct ofi_action action_info={};
	struct ofi_bucket_prop bucket_prop = { };
        char operation[30];
        uint8_t afterbucketid;
        uint32_t  command_bkt_id;
        uchar8_t offset;
        offset = OF_GROUP_TRANS_LISTNODE_OFFSET;

	CM_CBK_DEBUG_PRINT ("Entered");
	of_memset (&bucket_info, 0, sizeof (struct ofi_bucket));

	CM_CBK_DEBUG_PRINT ("trans id %d command id %d sub cmd id %d group id %d",trans_rec->trans_id,trans_rec->command_id,
			trans_rec->sub_command_id,trans_rec->group_id);

	if ((of_bucket_ucm_setmandparams (mand_iv_pairs,&group_info, &bucket_info, 
                          operation, &command_bkt_id, &of_bucket_result)) !=
			  OF_SUCCESS)
	{
		CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
		*result_pp=of_bucket_result;
		return OF_FAILURE;
	}

	return_value =of_bucket_ucm_setoptparams (opt_iv_pairs, &bucket_prop, &action_info,
                                  operation, &command_bkt_id, &of_bucket_result);
	if (return_value != OF_SUCCESS)
	{
		CM_CBK_DEBUG_PRINT ("Set Optional Parameters Failed");
		*result_pp =of_bucket_result;
		return OF_FAILURE;
	}
	OF_LIST_SCAN(group_trans_list_g, trans_rec, struct group_trans *,offset )
	{
		if(trans_rec->group_id == group_info.group_id)
		{
			CM_CBK_DEBUG_PRINT ("transaction found");
			break;
		}
	}

        group_info.group_type = trans_rec->group_type;
	CM_CBK_DEBUG_PRINT ("group type=%d", trans_rec->group_type);
#if 1
        if (strcmp(operation, "insert") == 0)
        {
          return_value = of_frame_and_send_mod_group_bucket_info(trans_rec->datapath_handle, 
                          &group_info, &bucket_info, &bucket_prop, &action_info, command_bkt_id);
          if (return_value == OF_SUCCESS)
            trans_rec->bucket_id = bucket_info.bucket_id;
        }
        else if (strcmp(operation, "remove") == 0)
        {
          if ((command_bkt_id == OFPG_BUCKET_FIRST) || (command_bkt_id == OFPG_BUCKET_LAST) ||
              (command_bkt_id == OFPG_BUCKET_ALL))
          {
            return_value = of_frame_and_send_del_bucket_info(trans_rec->datapath_handle, 
                          &group_info, &bucket_info, command_bkt_id);
          }
          else
          {
            return_value = of_frame_and_send_del_bucket_info(trans_rec->datapath_handle, 
                          &group_info, &bucket_info, bucket_info.bucket_id);
          }
       
          if (return_value == OF_SUCCESS)
            trans_rec->bucket_id = 0;
        }
        else
        {
	   CM_CBK_DEBUG_PRINT ("Invalid operation");
	   fill_app_result_struct (&of_bucket_result, NULL, CM_GLU_BUCKET_UPDATE_FAILED);
	   *result_pp =of_bucket_result;
           return OF_FAILURE;
        }

	if (return_value != OF_SUCCESS)
	{
		CM_CBK_DEBUG_PRINT ("Bucket updation at group %d failed", group_info.group_id);
		fill_app_result_struct (&of_bucket_result, NULL, CM_GLU_BUCKET_UPDATE_FAILED);
		*result_pp =of_bucket_result;
		return OF_FAILURE;
	}
#endif
	return OF_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_bucket_setrec (void * config_trans_p,
		struct cm_array_of_iv_pairs * mand_iv_pairs,
		struct cm_array_of_iv_pairs * opt_iv_pairs,
		struct cm_app_result ** result_pp)
{
	struct cm_app_result *of_bucket_result = NULL;
	int32_t return_value = OF_FAILURE;
	struct ofi_bucket bucket_info = { };
	struct ofi_group_desc_info group_info = { };
	struct group_trans *trans_rec=(struct group_trans *)config_trans_p;
        struct ofi_action action_info={};
	struct ofi_bucket_prop bucket_prop = { };
        char operation[30];
        uint8_t afterbucketid;
        uint32_t  command_bkt_id;
        uchar8_t offset;
        offset = OF_GROUP_TRANS_LISTNODE_OFFSET;

	CM_CBK_DEBUG_PRINT ("Entered");
	of_memset (&bucket_info, 0, sizeof (struct ofi_bucket));

	CM_CBK_DEBUG_PRINT ("trans id %d command id %d sub cmd id %d group id %d",trans_rec->trans_id,trans_rec->command_id,
			trans_rec->sub_command_id,trans_rec->group_id);

	if ((of_bucket_ucm_setmandparams (mand_iv_pairs,&group_info, &bucket_info, 
                          operation, &command_bkt_id, &of_bucket_result)) !=
			  OF_SUCCESS)
	{
		CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
		*result_pp=of_bucket_result;
		return OF_FAILURE;
	}

	return_value =of_bucket_ucm_setoptparams (opt_iv_pairs, &bucket_prop, &action_info,
                                  operation, &command_bkt_id, &of_bucket_result);
	if (return_value != OF_SUCCESS)
	{
		CM_CBK_DEBUG_PRINT ("Set Optional Parameters Failed");
		*result_pp =of_bucket_result;
		return OF_FAILURE;
	}
	OF_LIST_SCAN(group_trans_list_g, trans_rec, struct group_trans *,offset )
	{
		if(trans_rec->group_id == group_info.group_id)
		{
			CM_CBK_DEBUG_PRINT ("transaction found");
			break;
		}
	}

        group_info.group_type = trans_rec->group_type;
	CM_CBK_DEBUG_PRINT ("group type=%d", trans_rec->group_type);
#if 1
        /*if (strcmp(operation, "insert") == 0)
        {
          return_value = of_frame_and_send_mod_group_bucket_info(trans_rec->datapath_handle, 
                               &group_info, &bucket_info, &bucket_prop, &action_info, command_bkt_id);
          if (return_value == OF_SUCCESS)
            trans_rec->bucket_id = bucket_info.bucket_id;
        }
        else */
        if (strcmp(operation, "remove") == 0)
        {
          if ((command_bkt_id == OFPG_BUCKET_FIRST) || (command_bkt_id == OFPG_BUCKET_LAST) ||
              (command_bkt_id == OFPG_BUCKET_ALL))
          {
            return_value = of_frame_and_send_del_bucket_info(trans_rec->datapath_handle, 
                          &group_info, &bucket_info, command_bkt_id);
          }
          else
          {
            return_value = of_frame_and_send_del_bucket_info(trans_rec->datapath_handle, 
                          &group_info, &bucket_info, bucket_info.bucket_id);
          }

          if (return_value == OF_SUCCESS)
            trans_rec->bucket_id = 0;
        }
        else
        {
           /* assuming that action add so return success, if 
            any failure occure wile adding action, user will prompt with
            error message */
           return OF_SUCCESS;
        }

	if (return_value != OF_SUCCESS)
	{
		CM_CBK_DEBUG_PRINT ("Bucket updation at group %d failed", group_info.group_id);
		fill_app_result_struct (&of_bucket_result, NULL, CM_GLU_BUCKET_UPDATE_FAILED);
		*result_pp =of_bucket_result;
		return OF_FAILURE;
	}
#endif
	return OF_SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_bucket_delrec (void * config_trans_p,
		struct cm_array_of_iv_pairs * key_iv_pairs,
		struct cm_app_result ** result_pp)
{
	struct cm_app_result *of_bucket_result = NULL;
	int32_t return_value = OF_FAILURE;
	struct ofi_bucket bucket_info = { };
	struct ofi_group_desc_info group_info = { };
	struct group_trans *trans_rec=config_trans_p;
        uint8_t position;
        char operation[30];

	CM_CBK_DEBUG_PRINT ("Entered");
	of_memset (&bucket_info, 0, sizeof (struct ofi_bucket));

	if ( trans_rec->command_id != CM_CMD_ADD_PARAMS)
	{
		CM_CBK_DEBUG_PRINT ("Delete not supported");
		return OF_FAILURE;
	}

	if ((of_bucket_ucm_setmandparams (key_iv_pairs,&group_info, &bucket_info, operation, &position, &of_bucket_result)) !=
			OF_SUCCESS)
	{
		CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
		fill_app_result_struct (&of_bucket_result, NULL, CM_GLU_SET_MAND_PARAM_FAILED);
		*result_pp =of_bucket_result;
		return OF_FAILURE;
	}

	//return_value=of_group_unregister_bucket(trans_rec->datapath_handle,group_info.group_id, bucket_info.bucket_id);
	//return_value=of_inform_dp_abt_port_del_from_multicast_group(
         //           trans_rec->datapath_handle,group_info.group_id, bucket_info.bucket_id);
	if (return_value != OF_SUCCESS)
	{
		CM_CBK_DEBUG_PRINT ("Bucket does not exist with id %d",bucket_info.bucket_id);

		fill_app_result_struct (&of_bucket_result, NULL, CM_GLU_BUCKET_DOESNOT_EXIST);
		*result_pp =of_bucket_result;
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
int32_t  of_bucket_endtrans(void *config_trans_p,
		uint32_t command_id,
		struct cm_app_result ** result_pp)
{
	struct group_trans *trans_rec=config_trans_p;
	struct ofi_bucket group_info={};
	struct cm_app_result *of_bucket_result = NULL;

	CM_CBK_DEBUG_PRINT ("Entered");

	if ( trans_rec )
	{
		CM_CBK_DEBUG_PRINT ("trans id %d command id %d sub cmd id %d group id %d",trans_rec->trans_id,trans_rec->command_id,
				trans_rec->sub_command_id,trans_rec->group_id);
	}
	else
	{
		CM_CBK_DEBUG_PRINT ("trans is null");
	}

	switch(command_id)
	{

		case CM_CMD_CONFIG_SESSION_REVOKE:
			CM_CBK_DEBUG_PRINT ("Revoke command");
			break;
		case CM_CMD_CONFIG_SESSION_COMMIT:
			CM_CBK_DEBUG_PRINT ("Commit command");
			break;
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
int32_t of_bucket_getfirstnrecs(struct cm_array_of_iv_pairs * key_iv_pairs,
		uint32_t * pCount,
		struct cm_array_of_iv_pairs ** arr_ivpair_arr_pp)
{
	struct cm_array_of_iv_pairs *result_iv_pairs_p = NULL;
	int32_t return_value = OF_FAILURE;
	uint32_t uiRecCount = 0;
	struct cm_app_result *of_bucket_result = NULL;
	struct ofi_group_desc_info group_info={};
	struct ofi_bucket bucket_info={};
        uint8_t position;
        char operation[30];


	CM_CBK_DEBUG_PRINT ("not supporting");
	return OF_FAILURE;

	of_memset (&group_info, 0, sizeof (struct ofi_group_desc_info));
	of_memset (&bucket_info, 0, sizeof (struct ofi_bucket));
	if ((of_bucket_ucm_setmandparams (key_iv_pairs,&group_info, &bucket_info, operation, &position, &of_bucket_result)) !=
			OF_SUCCESS)
	{
		CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
		return OF_FAILURE;
	}


	//	return_value=cntlr_send_table_stats_request(1);
	if ( return_value != OF_SUCCESS)
	{
		CM_CBK_DEBUG_PRINT ("Table stats failed");
		return OF_FAILURE;
	} 
	CM_CBK_DEBUG_PRINT ("Table stats will be sent as a multipart response");
	return CM_CNTRL_MULTIPART_RESPONSE;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_bucket_getnextnrecs (struct cm_array_of_iv_pairs * key_iv_pairs,
		struct cm_array_of_iv_pairs *pPrevRecordKey, uint32_t * pCount,
		struct cm_array_of_iv_pairs ** pNextNRecordData_p)
{
	CM_CBK_DEBUG_PRINT ("no next records");
	*pCount=0;
	return OF_FAILURE;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int32_t of_bucket_getexactrec (struct cm_array_of_iv_pairs * key_iv_pairs,
		struct cm_array_of_iv_pairs ** pIvPairArr)
{
	struct cm_array_of_iv_pairs *result_iv_pairs_p = NULL;
	struct cm_app_result *of_bucket_result = NULL;
	uint32_t ii = 0;
	int32_t return_value = OF_FAILURE;
       struct ofi_group_desc_info group_info={};
       struct dprm_datapath_general_info datapath_info={};
       uint64_t datapath_handle;
       struct group_trans *trans_rec_entry_p=NULL;
       uchar8_t offset;
       offset=  OF_GROUP_TRANS_LISTNODE_OFFSET;
       char operation[30];
       uint32_t  command_bkt_id;
	struct ofi_bucket bucket_info = { };


	of_memset (&bucket_info, 0, sizeof (struct ofi_bucket));
	of_memset (&group_info, 0, sizeof (struct ofi_group_desc_info));

	if ((of_bucket_ucm_setmandparams (key_iv_pairs, &group_info, &bucket_info, 
                          operation, &command_bkt_id, &of_bucket_result)) !=
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
             if (trans_rec_entry_p->bucket_id == bucket_info.bucket_id)
             {
                result_iv_pairs_p =
                 (struct cm_array_of_iv_pairs *) of_calloc (1, sizeof (struct cm_array_of_iv_pairs));
               if (result_iv_pairs_p == NULL)
               {
                 CM_CBK_DEBUG_PRINT ("Memory allocation failed for result_iv_pairs_p");
                 return OF_FAILURE;
               }

               return_value = of_bucket_ucm_getparams (&bucket_info, &result_iv_pairs_p[0]);
               if ( return_value != OF_SUCCESS)
               {
                 CM_CBK_DEBUG_PRINT ("of_group_ucm_getparams failed");
                 return OF_FAILURE;
               } 

              *pIvPairArr = result_iv_pairs_p;
               return OF_SUCCESS;
             }
          }
        }
	CM_CBK_DEBUG_PRINT ("no exact record");
	return OF_FAILURE;

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Function Name:
 * Description:
 * Input:
 * Output:
 * Result:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
 int32_t of_bucket_ucm_setmandparams (struct cm_array_of_iv_pairs *mand_iv_pairs,
		struct  ofi_group_desc_info *group_info,
		struct ofi_bucket *bucket_info, char *operation, uint32_t *position,
		struct cm_app_result ** app_result_pp)
{
   uint32_t opt_param_cnt;
   uint32_t group_id;
   uint32_t bucket_id;

   CM_CBK_DEBUG_PRINT ("Entered");
   for (opt_param_cnt = 0; opt_param_cnt < mand_iv_pairs->count_ui;
			opt_param_cnt++)
   {
   	switch (mand_iv_pairs->iv_pairs[opt_param_cnt].id_ui)
   	{
           case CM_DM_GROUPDESC_GROUP_ID_ID:
      	      group_id=of_atoi((char *) mand_iv_pairs->iv_pairs[opt_param_cnt].value_p);
      	      group_info->group_id=group_id;
      	      CM_CBK_DEBUG_PRINT ("group id is %d", group_id);
      	   break;

   	   case  CM_DM_BUCKET_BUCKETID_ID:
      	      bucket_id=of_atoi((char *) mand_iv_pairs->iv_pairs[opt_param_cnt].value_p);
      	      bucket_info->bucket_id=bucket_id;
      	      CM_CBK_DEBUG_PRINT ("bucket id %d", bucket_id);
      	   break;

          case  CM_DM_BUCKET_BUCKETOPERATION_ID:
      	      strcpy(operation, (char *) mand_iv_pairs->iv_pairs[opt_param_cnt].value_p);
      	      CM_CBK_DEBUG_PRINT ("bucket operation %s", operation);
          break;


           case CM_DM_BUCKET_BUCKETPOSITION_ID:
             if((strcmp((char *) mand_iv_pairs->iv_pairs[opt_param_cnt].value_p, "first")) == 0)
             {
       	        *position=OFPG_BUCKET_FIRST;
	      CM_CBK_DEBUG_PRINT ("position %d", *position);
             }
             else if (strcmp((char *) mand_iv_pairs->iv_pairs[opt_param_cnt].value_p, "last") == 0)
             {
       	       *position=OFPG_BUCKET_LAST;
	      CM_CBK_DEBUG_PRINT ("position %d", *position);
             }
             else if(strcmp((char *) mand_iv_pairs->iv_pairs[opt_param_cnt].value_p,"all")==0)
             {
       	       *position=OFPG_BUCKET_ALL;
	      CM_CBK_DEBUG_PRINT ("position %d", *position);
             }
             else if(strcmp((char *) mand_iv_pairs->iv_pairs[opt_param_cnt].value_p,"afterbucket")==0)
             {
       	       //*position=OFPG_BUCKET_ALL;
	      CM_CBK_DEBUG_PRINT ("position %d", *position);
             }
             else 
             {
      	        CM_CBK_DEBUG_PRINT ("Invalid option %s", 
                            (char *) mand_iv_pairs->iv_pairs[opt_param_cnt].value_p);
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
 int32_t of_bucket_ucm_validatemandparams (struct cm_array_of_iv_pairs *
		mand_iv_pairs,
		struct cm_app_result ** app_result_pp)
{
	uint32_t opt_param_cnt;
	uint32_t group_id;
	uint8_t type;

	CM_CBK_DEBUG_PRINT ("Entered");
	for (opt_param_cnt = 0; opt_param_cnt < mand_iv_pairs->count_ui;
			opt_param_cnt++)
	{
		switch (mand_iv_pairs->iv_pairs[opt_param_cnt].id_ui)
		{
			case CM_DM_GROUPDESC_GROUP_ID_ID:
				group_id=of_atoi((char *) mand_iv_pairs->iv_pairs[opt_param_cnt].value_p);
				CM_CBK_DEBUG_PRINT ("group id is %d", group_id);
				break;
			case CM_DM_GROUPDESC_GROUPTYPE_ID:
				type=ucm_uint8_from_str_ptr((char *) mand_iv_pairs->iv_pairs[opt_param_cnt].value_p);
				CM_CBK_DEBUG_PRINT ("type is %d", type);
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
int32_t of_bucket_ucm_setoptparams (struct cm_array_of_iv_pairs *opt_iv_pairs,
                                   struct ofi_bucket_prop * bucket_prop,
                                   struct ofi_action * action_info,
                                   char *operation, uint32_t *afterbucketid,
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
       case  CM_DM_BUCKET_BUCKETOPERATION_ID:
      	      strcpy(operation, (char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
      	      CM_CBK_DEBUG_PRINT ("bucket operation %s", operation);
       break;

       case CM_DM_BUCKET_WEIGHT_ID:
              bucket_prop->weight=(uint16_t)of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
	     CM_CBK_DEBUG_PRINT ("weight  %d", bucket_prop->weight);
	break;

	case CM_DM_BUCKET_WATCH_PORT_ID:
		bucket_prop->watch_port=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		CM_CBK_DEBUG_PRINT ("watch_port  %d", bucket_prop->watch_port);
	break;
	case CM_DM_BUCKET_WATCH_GROUP_ID:
		bucket_prop->watch_group=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		CM_CBK_DEBUG_PRINT ("watch_group  %d", bucket_prop->watch_group);
	break;

#if 1
	case CM_DM_BUCKET_PORT_NO_ID:
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

	case CM_DM_BUCKET_MAX_LEN_ID:
		max_len=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		action_info->max_len=(uint16_t ) max_len;
		CM_CBK_DEBUG_PRINT ("max_len  %d action_info->max_len %d ", max_len,action_info->max_len);
	break;

	case CM_DM_BUCKET_TTL_ID:
		ttl=ucm_uint8_from_str_ptr((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		action_info->ttl= ttl;
		CM_CBK_DEBUG_PRINT ("ttl  %d action_info->ttl %d ", ttl,action_info->ttl);
		break;

	case CM_DM_BUCKET_ETHER_TYPE_ID:
		ether_type=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		action_info->ether_type=(uint16_t ) ether_type;
		CM_CBK_DEBUG_PRINT ("ether_type  %d action_info->ether_type %d ", ether_type,action_info->ether_type);
		break;

	case CM_DM_BUCKET_GROUPID_ID:
		group_id=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		action_info->group_id= group_id;
		CM_CBK_DEBUG_PRINT ("group_id  %d action_info->group_id %d ", group_id,action_info->group_id);
	break;

	case CM_DM_BUCKET_QUEUEID_ID:
		queue_id=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		action_info->queue_id= queue_id;
		CM_CBK_DEBUG_PRINT ("queue_id  %d action_info->queue_id %d ", queue_id,action_info->queue_id);
	break;

	case CM_DM_BUCKET_SETFIELDTYPE_ID:
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

	case CM_DM_BUCKET_SETFIELDVAL_ID:
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
	case  CM_DM_BUCKET_ACTIONTYPE_ID:
		action_type=((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
		action_type_len = (opt_iv_pairs->iv_pairs[uiOptParamCnt].value_length);
		of_group_set_action_type(action_info,action_type,action_type_len);

		//action_info->type=(uint16_t )action_type;
		CM_CBK_DEBUG_PRINT ("action type %d %s ",action_info->type,action_type);
         break;

#endif
         case CM_DM_BUCKET_BUCKETPOSITION_ID:
             if((strcmp((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p, "first")) == 0)
             {
       	        *afterbucketid=OFPG_BUCKET_FIRST;
	      CM_CBK_DEBUG_PRINT ("position %d", *afterbucketid);
             }
             else if (strcmp((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p, "last") == 0)
             {
       	       *afterbucketid=OFPG_BUCKET_LAST;
	      CM_CBK_DEBUG_PRINT ("position %d", *afterbucketid);
             }
             else if(strcmp((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p,"all")==0)
             {
       	       *afterbucketid=OFPG_BUCKET_ALL;
	      CM_CBK_DEBUG_PRINT ("position %d", *afterbucketid);
             }
             else if(strcmp((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p,"afterbucket")==0)
             {
       	       //*position=OFPG_BUCKET_ALL;
	      CM_CBK_DEBUG_PRINT ("position %d", *afterbucketid);
             }
             else 
             {
      	        CM_CBK_DEBUG_PRINT ("Invalid option %s", 
                            (char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
                return OF_FAILURE;
             }
	   break;

	case CM_DM_BUCKET_AFTERBUCKETID_ID:
            *afterbucketid=of_atoi((char *) opt_iv_pairs->iv_pairs[uiOptParamCnt].value_p);
	    CM_CBK_DEBUG_PRINT ("after bucket id = %d ", *afterbucketid);
	break;

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
 int32_t of_bucket_ucm_getparams (struct ofi_bucket *bucket_info,
		struct cm_array_of_iv_pairs * result_iv_pairs_p)
{
	char string[64];
	int32_t index = 0, jj, count_ui, iIndex;
#define CM_BUCKET_CHILD_COUNT 1
	count_ui = CM_BUCKET_CHILD_COUNT;
#if 1
	//CM_CBK_DEBUG_PRINT ("Entered");
	result_iv_pairs_p->iv_pairs = (struct cm_iv_pai*)of_calloc(count_ui, sizeof(struct cm_iv_pair));
	if(!result_iv_pairs_p->iv_pairs)
	{
		CM_CBK_DEBUG_PRINT (" Memory allocation failed ");
		return OF_FAILURE;
	}

	of_memset(string, 0, sizeof(string));
	of_itoa (bucket_info->bucket_id, string);
	FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_BUCKET_BUCKETID_ID, CM_DATA_TYPE_STRING, string);
	index++;

	/*of_memset(string, 0, sizeof(string));
	of_sprintf(string,"%d",bucket_info->group_type);
	FILL_CM_IV_PAIR(result_iv_pairs_p->iv_pairs[index],CM_DM_TABLESTATS_ACTIVECOUNT_ID,CM_DATA_TYPE_STRING, string);
	index++;*/


	result_iv_pairs_p->count_ui = index;

	//CM_CBK_PRINT_IVPAIR_ARRAY (result_iv_pairs_p);
#endif
	return OF_SUCCESS;
}
#endif /*OF_CM_SUPPORT */
