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
 * File name: vnattributecbk.c
 * 
 */

#ifdef OF_CM_SUPPORT


#include "cbcmninc.h"
#include "cmgutil.h"
#include "cntrucminc.h"
#include "crmapi.h"
#include "crm_vn_api.h"


int32_t crm_vn_attribute_appl_ucmcbk_init (void);

int32_t crm_vn_attribute_addrec (void * config_trans_p,
			struct cm_array_of_iv_pairs * pMandParams,
			struct cm_array_of_iv_pairs * pOptParams,
			struct cm_app_result ** result_p);
int32_t crm_vn_attribute_setrec (void * config_trans_p,
			struct cm_array_of_iv_pairs * pMandParams,
			struct cm_array_of_iv_pairs * pOptParams,
			struct cm_app_result ** result_p);




int32_t crm_vn_attribute_delrec (void * config_transaction_p,
			struct cm_array_of_iv_pairs * keys_arr_p,
			struct cm_app_result ** result_p);

int32_t crm_vn_attribute_verifycfg (struct cm_array_of_iv_pairs * keys_arr_p,
			uint32_t command_id, struct cm_app_result ** result_p);

int32_t crm_vn_attribute_getfirstnrecs(struct cm_array_of_iv_pairs * keys_arr_p,
                        uint32_t * count_p,
                        struct cm_array_of_iv_pairs ** array_of_iv_pair_arr_p);

int32_t crm_vn_attribute_getnextnrecs (struct cm_array_of_iv_pairs * keys_arr_p,
                        struct cm_array_of_iv_pairs *prev_record_key_p, uint32_t * count_p,
                        struct cm_array_of_iv_pairs ** next_n_record_data_p);

int32_t crm_vn_attribute_getexactrec (struct cm_array_of_iv_pairs * keys_arr_p,
    struct cm_array_of_iv_pairs ** pIvPairArr);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * attribute  U T I L I T Y  F U N C T I O N S * * * * * * * * * * *
 * * * * * * * * * * * * * **  * * * * * * * **  * * * * * * * * * * * * * */

int32_t crm_vn_attribute_ucm_validatemandparams (struct cm_array_of_iv_pairs *pMandParams,
			struct cm_app_result **presult_p);



int32_t crm_vn_attribute_ucm_setmandparams (struct cm_array_of_iv_pairs *pMandParams,
			char *vn_name_p,
			struct crm_attribute_name_value_pair *attribute_info,
			struct cm_app_result                  **presult_p);






struct cm_dm_app_callbacks crm_vn_attribute_ucm_callbacks = 
{
  NULL,
  crm_vn_attribute_addrec,
  crm_vn_attribute_setrec,
  crm_vn_attribute_delrec,
  NULL,
  crm_vn_attribute_getfirstnrecs,
  crm_vn_attribute_getnextnrecs,
  crm_vn_attribute_getexactrec,
  crm_vn_attribute_verifycfg,
  NULL
};

int32_t crm_vn_attribute_appl_ucmcbk_init (void)
{
  CM_CBK_DEBUG_PRINT ("Entry");
  cm_register_app_callbacks (CM_ON_DIRECTOR_CRM_VIRTUALNETWORK_VNATTRIBUTE_APPL_ID, &crm_vn_attribute_ucm_callbacks);
  return OF_SUCCESS;
}

int32_t crm_vn_attribute_addrec (void * config_trans_p,
		  		     struct cm_array_of_iv_pairs * pMandParams,
			             struct cm_array_of_iv_pairs * pOptParams,
			             struct cm_app_result ** result_p)
{
  struct   cm_app_result *attribute_result = NULL;
  struct   crm_attribute_name_value_pair crm_attribute_name_value_info={};
  char vn_name[CRM_MAX_VN_NAME_LEN+1]={};

  uint64_t vn_handle;
  int32_t  return_value = OF_FAILURE,result;

  CM_CBK_DEBUG_PRINT ("Entered");

  of_memset (&crm_attribute_name_value_info, 0, sizeof (struct crm_attribute_name_value_pair));


  if ((crm_vn_attribute_ucm_setmandparams (pMandParams, 
	      vn_name, &crm_attribute_name_value_info, 
	      &attribute_result)) != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    fill_app_result_struct (&attribute_result, NULL, CM_GLU_SET_MAND_PARAM_FAILED);
    *result_p = attribute_result;
    return OF_FAILURE;
  }



  return_value = crm_get_vn_handle(vn_name, &vn_handle);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("vn does not exist with name %s",vn_name);
    fill_app_result_struct (&attribute_result, NULL, CM_GLU_VN_NAME_ID_INVALID);
    *result_p = attribute_result;
    return OF_FAILURE;
  }
  return_value = crm_add_attribute_to_vn(vn_handle, &crm_attribute_name_value_info);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Attribute Modification Failed(ret:%d",return_value);
    fill_app_result_struct (&attribute_result, NULL, CM_GLU_ATTRIBUTE_ADD_FAILED);
    *result_p = attribute_result;
    return OF_FAILURE;
  }
  CM_CBK_DEBUG_PRINT (" Attribute added succesfully");
  return OF_SUCCESS;

}
int32_t crm_vn_attribute_setrec (void * config_trans_p,
		  		     struct cm_array_of_iv_pairs * pMandParams,
			             struct cm_array_of_iv_pairs * pOptParams,
			             struct cm_app_result ** result_p)
{
  struct   cm_app_result *attribute_result = NULL;
  struct   crm_attribute_name_value_pair crm_attribute_name_value_info={};
  char vn_name[CRM_MAX_VN_NAME_LEN+1]={};

  uint64_t vn_handle;
  int32_t  return_value = OF_FAILURE,result;

  CM_CBK_DEBUG_PRINT ("Entered");

  of_memset (&crm_attribute_name_value_info, 0, sizeof (struct crm_attribute_name_value_pair));


  if ((crm_vn_attribute_ucm_setmandparams (pMandParams, 
	      vn_name, &crm_attribute_name_value_info, 
	      &attribute_result)) != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    fill_app_result_struct (&attribute_result, NULL, CM_GLU_SET_MAND_PARAM_FAILED);
    *result_p = attribute_result;
    return OF_FAILURE;
  }



  return_value = crm_get_vn_handle(vn_name, &vn_handle);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("vn does not exist with name %s",vn_name);
    fill_app_result_struct (&attribute_result, NULL, CM_GLU_VN_NAME_ID_INVALID);
    *result_p = attribute_result;
    return OF_FAILURE;
  }
  return_value = crm_add_attribute_to_vn(vn_handle, &crm_attribute_name_value_info);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Attribute modification Failed(ret:%d",return_value);
    fill_app_result_struct (&attribute_result, NULL, CM_GLU_ATTRIBUTE_ADD_FAILED);
    *result_p = attribute_result;
    return OF_FAILURE;
  }
  CM_CBK_DEBUG_PRINT ("Attribute modified succesfully");
  return OF_SUCCESS;

}




int32_t crm_vn_attribute_getnextnrecs (struct cm_array_of_iv_pairs * keys_arr_p,
                        struct cm_array_of_iv_pairs *prev_record_key_p, uint32_t * count_p,
                        struct cm_array_of_iv_pairs ** next_n_record_data_p)
{
  struct   cm_array_of_iv_pairs *result_iv_pairs_p = NULL;
  struct   crm_attribute_name_value_pair        crm_attribute_name_value_info={};
  struct   crm_attribute_name_value_output_info crm_attribute_output={};
  char     current_name_string[CRM_MAX_ATTRIBUTE_NAME_LEN];
  char     current_value_string[CRM_MAX_ATTRIBUTE_VALUE_LEN];
  char     vn_name[CRM_MAX_VN_NAME_LEN+1]={};
  uint64_t vn_handle;

  CM_CBK_DEBUG_PRINT ("Entered");

  uint64_t datapath_handle;
  int32_t return_value = OF_FAILURE;
  uint32_t uiRecCount = 0;
  uint32_t uindex_i = 0;

  of_memset (&crm_attribute_name_value_info, 0, sizeof (struct crm_attribute_name_value_pair));
  if ((crm_vn_attribute_ucm_setmandparams (prev_record_key_p, 
		vn_name, &crm_attribute_name_value_info, 
					NULL)) != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    return OF_FAILURE;
  }
  if ((crm_vn_attribute_ucm_setmandparams (keys_arr_p, 
		vn_name, &crm_attribute_name_value_info, 
					NULL)) != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
    return OF_FAILURE;
  }


  return_value = crm_get_vn_handle(vn_name, &vn_handle);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("vn does not exist with name %s",vn_name);
    return OF_FAILURE;
  }
  crm_attribute_output.name_length = CRM_MAX_ATTRIBUTE_NAME_LEN -1;
  crm_attribute_output.value_length = CRM_MAX_ATTRIBUTE_VALUE_LEN -1;

  return_value = crm_get_vn_next_attribute(vn_handle, crm_attribute_name_value_info.name_string,&crm_attribute_output);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("crm_get_vn_next_attribute_name failed!");
     return OF_FAILURE;
  }
  result_iv_pairs_p = (struct cm_array_of_iv_pairs *) of_calloc (1, sizeof (struct cm_array_of_iv_pairs));
  if (result_iv_pairs_p == NULL)
  {
    CM_CBK_DEBUG_PRINT ("Memory allocation failed for result_iv_pairs_p");
    return OF_FAILURE;
  }
  crm_attribute_vn_getparams(&crm_attribute_output, &result_iv_pairs_p[uiRecCount]);
  uiRecCount++;
  *next_n_record_data_p = result_iv_pairs_p;
  *count_p = uiRecCount;	
  return CRM_SUCCESS;  
}

int32_t crm_vn_attribute_getfirstnrecs(struct cm_array_of_iv_pairs * keys_arr_p,
		uint32_t * count_p,
		struct cm_array_of_iv_pairs ** array_of_iv_pair_arr_p)
{
  struct cm_array_of_iv_pairs *result_iv_pairs_p = NULL;
  struct   crm_attribute_name_value_pair crm_attribute_name_value_info={};
  struct   crm_attribute_name_value_output_info crm_attribute_output={};
  char vn_name[CRM_MAX_VN_NAME_LEN+1]={};

  uint64_t vn_handle;
  int32_t return_value = OF_FAILURE;
  uint32_t uiRecCount = 0;

  CM_CBK_DEBUG_PRINT ("Entered");

  of_memset (&crm_attribute_name_value_info, 0, sizeof (struct crm_attribute_name_value_pair));
  if ((crm_vn_attribute_ucm_setmandparams (keys_arr_p, vn_name, &crm_attribute_name_value_info, 
					NULL)) != OF_SUCCESS)
   {
     CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
     return OF_FAILURE;
  }
  return_value = crm_get_vn_handle(vn_name, &vn_handle);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Subnet does not exist with id %s",vn_name);
    return OF_FAILURE;
  }

  crm_attribute_output.name_length = CRM_MAX_ATTRIBUTE_NAME_LEN -1;
  crm_attribute_output.value_length = CRM_MAX_ATTRIBUTE_VALUE_LEN -1;

  return_value = crm_get_vn_first_attribute(vn_handle, &crm_attribute_output);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("crm_get_first_vn_attribute_name failed!");
    return OF_FAILURE;
  }
  result_iv_pairs_p = (struct cm_array_of_iv_pairs *) of_calloc (1, sizeof (struct cm_array_of_iv_pairs));
  if (result_iv_pairs_p == NULL)
  {
    CM_CBK_DEBUG_PRINT ("Memory allocation failed for result_iv_pairs_p");
    return OF_FAILURE;
  }
  crm_attribute_vn_getparams(&crm_attribute_output, &result_iv_pairs_p[uiRecCount]);
  uiRecCount++;
  *array_of_iv_pair_arr_p = result_iv_pairs_p;
  *count_p = uiRecCount;  
  return CRM_SUCCESS;  
}

int32_t crm_vn_attribute_getexactrec (struct cm_array_of_iv_pairs * keys_arr_p,
    struct cm_array_of_iv_pairs ** pIvPairArr)

{
  struct   cm_array_of_iv_pairs *result_iv_pairs_p = NULL;
  int32_t  return_value = OF_FAILURE;
  struct   crm_attribute_name_value_pair crm_attribute_name_value_info={};
  char     vn_name[CRM_MAX_VN_NAME_LEN+1]={};
  uint64_t vn_handle;

  CM_CBK_DEBUG_PRINT ("Entered");

  result_iv_pairs_p =
                     (struct cm_array_of_iv_pairs *) of_calloc (1, sizeof (struct cm_array_of_iv_pairs));
  if (result_iv_pairs_p == NULL)
   {
     CM_CBK_DEBUG_PRINT ("Failed to allocate memory for result_iv_pairs_p");
     return OF_FAILURE;
   }


  of_memset (&crm_attribute_name_value_info, 0, sizeof (struct crm_attribute_name_value_pair));

  if ((crm_vn_attribute_ucm_setmandparams (keys_arr_p,vn_name, &crm_attribute_name_value_info, 
	      NULL)) != OF_SUCCESS)
   {
     CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
     return OF_FAILURE;
   }
   return_value = crm_get_vn_handle(vn_name, &vn_handle);
   if (return_value != OF_SUCCESS)
    {
      CM_CBK_DEBUG_PRINT ("vn does not exist with name %s",vn_name);
      return OF_FAILURE;
    }

  return_value = crm_get_vn_exact_attribute(vn_handle, crm_attribute_name_value_info.name_string);
    if (return_value != OF_SUCCESS)
     {
       CM_CBK_DEBUG_PRINT ("Unable to find  exact attribute record!");
       return OF_FAILURE;
     }

  crm_attribute_vn_getparams (&crm_attribute_name_value_info, result_iv_pairs_p);
  *pIvPairArr = result_iv_pairs_p;
  return return_value;
}

int32_t crm_attribute_vn_getparams (struct crm_attribute_name_value_output_info *attribute_info,
		struct cm_array_of_iv_pairs * result_iv_pairs_p)
{
  uint32_t uindex_i = 0;
  char buf[128] = "";

  CM_CBK_DEBUG_PRINT ("Entered");
  result_iv_pairs_p->iv_pairs = (struct cm_iv_pair *) of_calloc (2, sizeof (struct cm_iv_pair));
  if (result_iv_pairs_p->iv_pairs == NULL)
   {
     CM_CBK_DEBUG_PRINT("Memory allocation failed for result_iv_pairs_p->iv_pairs");
     return OF_FAILURE;
   }

  FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_VNATTRIBUTE_ATTRIBUTENAME_ID,
			CM_DATA_TYPE_STRING, attribute_info->name_string);
  uindex_i++;

  FILL_CM_IV_PAIR (result_iv_pairs_p->iv_pairs[uindex_i],CM_DM_VNATTRIBUTE_ATTRIBUTEVAL_ID,
			CM_DATA_TYPE_STRING, attribute_info->value_string);
  uindex_i++;

  result_iv_pairs_p->count_ui = uindex_i;
  return OF_SUCCESS;   

}


int32_t crm_vn_attribute_ucm_setmandparams (struct cm_array_of_iv_pairs *pMandParams,
			char *vn_name_p,
			struct crm_attribute_name_value_pair *attribute_info,
			struct cm_app_result              **presult_p)
{
   uint32_t uiMandParamCnt;
   uint64_t idpId;
   uint32_t iportId;

   CM_CBK_DEBUG_PRINT ("Entered");
   for (uiMandParamCnt = 0; uiMandParamCnt < pMandParams->count_ui;
				 uiMandParamCnt++)
   {
     switch (pMandParams->iv_pairs[uiMandParamCnt].id_ui)
     {
       case CM_DM_VIRTUALNETWORK_NAME_ID:
             of_strncpy (vn_name_p,(char *) pMandParams->iv_pairs[uiMandParamCnt].value_p,pMandParams->iv_pairs[uiMandParamCnt].value_length);
	     CM_CBK_DEBUG_PRINT ("vn_name %s",vn_name_p);
             break;


      case CM_DM_VNATTRIBUTE_ATTRIBUTENAME_ID:
	    of_strncpy (attribute_info->name_string,(char *) pMandParams->iv_pairs[uiMandParamCnt].value_p,pMandParams->iv_pairs[uiMandParamCnt].value_length);
            CM_CBK_DEBUG_PRINT ("Attribute name %s",attribute_info->name_string);
	    break;

     case CM_DM_VNATTRIBUTE_ATTRIBUTEVAL_ID:
	   of_strncpy (attribute_info->value_string,(char *) pMandParams->iv_pairs[uiMandParamCnt].value_p,pMandParams->iv_pairs[uiMandParamCnt].value_length);
	   CM_CBK_DEBUG_PRINT ("Attribute value %s",attribute_info->value_string);
	   break;
     }
   }
	 CM_CBK_PRINT_IVPAIR_ARRAY (pMandParams);
	 return OF_SUCCESS;
}


int32_t crm_vn_attribute_verifycfg (struct cm_array_of_iv_pairs * keys_arr_p,
			uint32_t command_id, struct cm_app_result ** result_p)
{
  struct cm_app_result *attribute_result = NULL;
  int32_t return_value = OF_FAILURE;

  CM_CBK_DEBUG_PRINT ("Entered");

  return_value = crm_vn_attribute_ucm_validatemandparams (keys_arr_p, &attribute_result);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Validate Mandatory Parameters Failed");
    return OF_FAILURE;
  }
  *result_p = attribute_result;
  return OF_SUCCESS;
}

int32_t crm_vn_attribute_ucm_validatemandparams (struct cm_array_of_iv_pairs *
			pMandParams,	struct cm_app_result **presult_p)
{
  uint32_t uiMandParamCnt;
  struct cm_app_result *attribute_result = NULL;
  uint32_t uiPortId;


  CM_CBK_DEBUG_PRINT ("Entered");
  for (uiMandParamCnt = 0; uiMandParamCnt < pMandParams->count_ui; uiMandParamCnt++)
  {
    switch (pMandParams->iv_pairs[uiMandParamCnt].id_ui)
    {
      case CM_DM_VNATTRIBUTE_ATTRIBUTENAME_ID:
        if (pMandParams->iv_pairs[uiMandParamCnt].value_p == NULL)
	{
	  CM_CBK_DEBUG_PRINT ("Attribute Name is NULL");
	  fill_app_result_struct (&attribute_result, NULL, CM_ATTRIBUTE_NAME_NULL);
	  *presult_p = attribute_result;
	  return OF_FAILURE;
	}
	break;
      case CM_DM_VNATTRIBUTE_ATTRIBUTEVAL_ID: 
	if (pMandParams->iv_pairs[uiMandParamCnt].value_p == NULL)
        {
	  CM_CBK_DEBUG_PRINT ("Attribute Value is NULL");
	  fill_app_result_struct (&attribute_result, NULL, CM_ATTRIBUTE_VAL_NULL);
	  *presult_p = attribute_result;
	  return OF_FAILURE;
	}
	break;
      }
  }
  CM_CBK_PRINT_IVPAIR_ARRAY (pMandParams);
  return OF_SUCCESS;
}
int32_t crm_vn_attribute_delrec (void * config_transaction_p,
			struct cm_array_of_iv_pairs * keys_arr_p,
			struct cm_app_result ** result_p)
{
  struct cm_app_result *attribute_result = NULL;
  int32_t return_value = OF_FAILURE; 
  struct crm_attribute_name_value_pair attribute_info={};
  uint64_t vn_handle;
  char vn_name[CRM_MAX_VN_NAME_LEN+1]={};

  CM_CBK_DEBUG_PRINT ("Entered");
  of_memset (&attribute_info, 0, sizeof (struct crm_attribute_name_value_pair));

  if ((crm_vn_attribute_ucm_setmandparams (keys_arr_p, vn_name, &attribute_info, &attribute_result)) != OF_SUCCESS)
   {
     CM_CBK_DEBUG_PRINT ("Set Mandatory Parameters Failed");
     fill_app_result_struct (&attribute_result, NULL, CM_GLU_SET_MAND_PARAM_FAILED);
     *result_p = attribute_result;
     return OF_FAILURE;
     
   }

  return_value=crm_get_vn_handle(vn_name, &vn_handle);
  if (return_value != OF_SUCCESS)
   {
     CM_CBK_DEBUG_PRINT ("Get vn handle failed");
     fill_app_result_struct (&attribute_result, NULL, CM_GLU_VN_NAME_ID_INVALID);
     *result_p = attribute_result;
     return OF_FAILURE;
   }
  return_value=crm_delete_attribute_from_vn(vn_handle, &attribute_info);
  if (return_value != OF_SUCCESS)
  {
    CM_CBK_DEBUG_PRINT ("Delete attribute record failed");
    fill_app_result_struct (&attribute_result, NULL, CM_GLU_ATTRIBUTE_DEL_FAILED);
    *result_p = attribute_result;
    return OF_FAILURE;
  }

  CM_CBK_DEBUG_PRINT ("vn attribute deleted succesfully");
  return OF_SUCCESS;
}
#endif
