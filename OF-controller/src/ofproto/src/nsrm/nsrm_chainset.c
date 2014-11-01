
/** Network Service resource manager  source file 
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

/*
 *
 * File name:nsrm_chainset.c
 * Author: P.Vinod Sarma <B46178@freescale.com>
 * Date:   26/01/2014
 * Description: 
*/

#include  "controller.h"
#include  "nsrm.h"
#include  "nsrmldef.h"
#include  "crmapi.h"

extern void     *nsrm_nschainset_object_mempool_g;
extern void     *nsrm_nschainset_nwservice_object_mempool_g;
extern uint32_t nsrm_no_of_nschainset_object_buckets_g;
extern void     *nsrm_notifications_mempool_g;
extern struct   mchash_table* nsrm_nschainset_object_table_g ;

extern  of_list_t nsrm_nschainset_object_notifications[];



/***********************************************************************************************/
int32_t nsrm_get_nschainset_object_handle(char *nschainset_object_name_p, uint64_t *nschainset_object_handle_p)
{
   uint64_t hashkey;
   int32_t offset;
   struct nsrm_nschainset_object* nschainset_object_entry_p = NULL;
   if(nsrm_nschainset_object_table_g == NULL)
   {
     OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "There is no hash table!.");
     return NSRM_FAILURE;
   }
   hashkey = nsrm_get_hashval_byname(nschainset_object_name_p, nsrm_no_of_nschainset_object_buckets_g);
   CNTLR_RCU_READ_LOCK_TAKE();
   offset = NSRM_NSCHAINSET_OBJECT_TBL_OFFSET;
   MCHASH_BUCKET_SCAN(nsrm_nschainset_object_table_g, hashkey, nschainset_object_entry_p,struct nsrm_nschainset_object*,offset)
   {
     if(strcmp(nschainset_object_entry_p->name_p, nschainset_object_name_p))
     {
       continue;
     }
     *nschainset_object_handle_p = nschainset_object_entry_p->magic;
     *nschainset_object_handle_p = ((*nschainset_object_handle_p<<32) | (nschainset_object_entry_p->index));
     CNTLR_RCU_READ_LOCK_RELEASE();
     return NSRM_SUCCESS;
  }
    CNTLR_RCU_READ_LOCK_RELEASE();
    OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "nschainset object  name not found!.");
    return NSRM_ERROR_NSCHAINSET_OBJECT_NAME_INVALID;
}
/******************************************************************************************************/
int32_t nsrm_get_nschainset_byhandle(uint64_t nschainset_handle,struct nsrm_nschainset_object
                                                              **nschainset_object_p_p)
{
  /* Caller shall take RCU locks. */
  uint32_t index,magic;
  uint8_t status_b;

  magic = (uint32_t)(nschainset_handle >>32);
  index = (uint32_t)nschainset_handle;

  MCHASH_ACCESS_NODE(nsrm_nschainset_object_table_g,index,magic,*nschainset_object_p_p,status_b);
  if(TRUE == status_b)
    return NSRM_SUCCESS;
  return NSRM_FAILURE;
}
/***************************************************************************************************/
int32_t nsrm_add_nschainset_object(
        int32_t number_of_config_params,
        struct  nsrm_nschainset_object_key*         key_info_p,
        struct  nsrm_nschainset_object_config_info* config_info_p)
{
  
   struct nsrm_nschainset_object   *nschainset_object_p = NULL;
   struct nsrm_nschainset_object   *nschainset_object_node_scan_p = NULL;
   struct crm_tenant_config_info   crm_tenant_config_info_p = {};
   union  nsrm_nschainset_object_notification_data  notification_data = {};
   struct nsrm_notification_app_hook_info *app_entry_p = NULL;

   uint32_t  hashkey,apphookoffset,index = 0,magic = 0;
   uint8_t   heap_b, status_b = FALSE;
   int32_t   status = NSRM_SUCCESS,ret_value;
   uchar8_t* hashobj_p = NULL;
   uint64_t  tenant_handle,offset;
   uint64_t  nschainset_object_handle;

   if(nsrm_nschainset_object_table_g == NULL)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "No nschainset object hash table")
      return NSRM_FAILURE;
   }

   if(key_info_p->name_p == NULL)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "nschainset object name is null");
      return NSRM_ERROR_NSCHAINSET_OBJECT_NAME_INVALID;
   }
    
   ret_value = crm_get_tenant_handle(key_info_p->tenant_name_p,&tenant_handle);
   if(ret_value != NSRM_SUCCESS)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "Tenant doesn't exist . So create tenant");
      strcpy(crm_tenant_config_info_p.tenant_name , key_info_p->tenant_name_p); 
      ret_value = crm_add_tenant(&crm_tenant_config_info_p , &tenant_handle);
      if(ret_value != NSRM_SUCCESS)
      {
         OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR,"Tenant could not be added");
         return NSRM_FAILURE;
      }
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG,"tenant added is :%s",crm_tenant_config_info_p.tenant_name);

   }

   if(number_of_config_params == 0)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR,"No parameters to be configured");
      return NSRM_FAILURE;
   } 
  
   CNTLR_RCU_READ_LOCK_TAKE();
   
   /*Calculate hash value*/
   hashkey = nsrm_get_hashval_byname(key_info_p->name_p,nsrm_no_of_nschainset_object_buckets_g);
   offset = NSRM_NSCHAINSET_OBJECT_TBL_OFFSET;
   do
   {
      MCHASH_BUCKET_SCAN(nsrm_nschainset_object_table_g ,hashkey ,nschainset_object_node_scan_p,
                         struct nsrm_nschainset_object* ,offset)
      {
        if(strcmp(key_info_p->name_p,nschainset_object_node_scan_p->name_p) !=0)
          continue;
        OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR,"Duplicate nschainset object name : %s",key_info_p->name_p);
        status = NSRM_ERROR_DUPLICATE_RESOURCE;
        break;
      }
     ret_value = mempool_get_mem_block(nsrm_nschainset_object_mempool_g,
                                       ( uchar8_t **)&nschainset_object_p,&heap_b);
     if(ret_value!=MEMPOOL_SUCCESS)
     {
        OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "Failed to get memory block");
        status =  NSRM_FAILURE;
        break;
     }
     nschainset_object_p->heap_b = heap_b;
     nschainset_object_p->name_p = (char*)calloc(1,(strlen(key_info_p->name_p)+1));
     nschainset_object_p->tenant_name_p = (char*)calloc(1,(strlen(key_info_p->tenant_name_p)+1));
     strcpy(nschainset_object_p->name_p , key_info_p->name_p);
     strcpy(nschainset_object_p->tenant_name_p , key_info_p->tenant_name_p);
     nschainset_object_p->nschainset_type    = config_info_p[0].value.nschainset_type;
     nschainset_object_p->admin_status_e    =  config_info_p[1].value.admin_status_e;
  

     hashobj_p = (uchar8_t *)nschainset_object_p + NSRM_NSCHAINSET_OBJECT_TBL_OFFSET;
     MCHASH_APPEND_NODE(nsrm_nschainset_object_table_g, hashkey, nschainset_object_p,index, magic, hashobj_p, status_b);
 
     if(FALSE == status_b)
     {
        OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR," Failed to append nschainset object node to Hash table");
        status = NSRM_NSCHAINSET_OBJECT_ADD_FAIL;
        break;
     }
     nschainset_object_handle   = ((nschainset_object_handle<<32) | (index));
     nschainset_object_p->magic = magic;
     nschainset_object_p->index = index;
     nschainset_object_p->nschainset_object_handle = nschainset_object_handle;
   
     apphookoffset = NSRM_NOTIFICATION_APP_HOOK_OFFSET;
     notification_data.add_del.nschainset_object_name_p = (char*)calloc(1,(strlen(nschainset_object_p->name_p)+1));
     strcpy(notification_data.add_del.nschainset_object_name_p ,nschainset_object_p->name_p);
     notification_data.add_del.nschainset_object_handle = nschainset_object_p->nschainset_object_handle;
    
     OF_LIST_SCAN(nsrm_nschainset_object_notifications[NSRM_NSCHAIN_SET_OBJECT_ADDED], 
                  app_entry_p,
                  struct nsrm_notification_app_hook_info *,
                  apphookoffset)
     {
        ((nsrm_nschainset_object_notifier)(app_entry_p->call_back))(NSRM_NSCHAIN_SET_OBJECT_ADDED,
                                                                   nschainset_object_handle,
                                                                   notification_data,
                                                                   app_entry_p->cbk_arg1,
                                                                   app_entry_p->cbk_arg2
                                                                   );
     }
   
     OF_LIST_SCAN(nsrm_nschainset_object_notifications[NSRM_NSCHAIN_SET_OBJECT_ALL] ,
                  app_entry_p,
                  struct nsrm_notification_app_hook_info *,
                  apphookoffset)
     {
        ((nsrm_nschainset_object_notifier)(app_entry_p->call_back))(NSRM_NSCHAIN_SET_OBJECT_ADDED,
                                                                   nschainset_object_handle,
                                                                   notification_data,
                                                                   app_entry_p->cbk_arg1,
                                                                   app_entry_p->cbk_arg2
                                                                   );
     }
    
     CNTLR_RCU_READ_LOCK_RELEASE();
     OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "nschainset object added successfully to the hashtable.");
     return NSRM_SUCCESS;
}while(0);

    if(status != NSRM_SUCCESS)
    {
      free(nschainset_object_p->name_p);
      free(nschainset_object_p->tenant_name_p);
      if(nschainset_object_p)
      {
        if(mempool_release_mem_block(nsrm_nschainset_object_mempool_g,(uchar8_t*)&nschainset_object_p,
                                     nschainset_object_p->heap_b) != MEMPOOL_SUCCESS)

        { 
           OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "Failed to release  allocated memory block.");
        }

     }
   }
  CNTLR_RCU_READ_LOCK_RELEASE();
  return status;
}

 
int32_t nsrm_del_nschainset_object(
        struct nsrm_nschainset_object_key*   key_info_p)

{
   struct nsrm_nschainset_object   *nschainset_object_p = NULL;
   union  nsrm_nschainset_object_notification_data  notification_data = {};
   struct nsrm_notification_app_hook_info *app_entry_p = NULL;

   uint32_t  index,magic;
   int32_t   lstoffset,offset,ret_value;
   uint8_t   status_b = FALSE;
   uint64_t  nschainset_object_handle;

   if(nsrm_nschainset_object_table_g == NULL)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR,"nschainset hash table doesnt exist");
      return NSRM_FAILURE;
   }

   if(key_info_p->name_p == NULL)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR,"nschainset object name is invalid");
      return NSRM_ERROR_NSCHAINSET_OBJECT_NAME_INVALID;
   }
 
   ret_value = nsrm_get_nschainset_object_handle(key_info_p->name_p ,&nschainset_object_handle);
   if(ret_value != NSRM_SUCCESS)
   {
     OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR,"nschainset object doesnt exist");
     return NSRM_ERROR_NSCHAINSET_OBJECT_NAME_INVALID;
   }
   magic = (uint32_t)(nschainset_object_handle >>32);
   index = (uint32_t)nschainset_object_handle;

   CNTLR_RCU_READ_LOCK_TAKE();

   MCHASH_ACCESS_NODE(nsrm_nschainset_object_table_g,index,magic,nschainset_object_p,status_b);
   if(FALSE == status_b)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR,"no node found");
      CNTLR_RCU_READ_LOCK_RELEASE();
      return NSRM_FAILURE;
   }
   if(strcmp(key_info_p->tenant_name_p,nschainset_object_p->tenant_name_p) != 0)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR,"This nschainset is not present in this tenant");
      return NSRM_FAILURE;
   }
   lstoffset = NSRM_NOTIFICATION_APP_HOOK_OFFSET;
   notification_data.add_del.nschainset_object_name_p = (char*)calloc(1,(strlen(nschainset_object_p->name_p)+1));
   strcpy(notification_data.add_del.nschainset_object_name_p ,nschainset_object_p->name_p);
   notification_data.add_del.nschainset_object_handle = nschainset_object_p->nschainset_object_handle;

   OF_LIST_SCAN(nsrm_nschainset_object_notifications[NSRM_NSCHAIN_SET_OBJECT_DELETED] ,
                  app_entry_p,
                  struct nsrm_notification_app_hook_info *,
                  lstoffset)
   {
      ((nsrm_nschainset_object_notifier)(app_entry_p->call_back))(NSRM_NSCHAIN_SET_OBJECT_DELETED,
                                                                   nschainset_object_handle,
                                                                   notification_data,
                                                                   app_entry_p->cbk_arg1,
                                                                   app_entry_p->cbk_arg2
                                                                   );
   } 
   
   OF_LIST_SCAN(nsrm_nschainset_object_notifications[NSRM_NSCHAIN_SET_OBJECT_ALL] ,
                  app_entry_p,
                  struct nsrm_notification_app_hook_info *,
                  lstoffset)
   {
      ((nsrm_nschainset_object_notifier)(app_entry_p->call_back))(NSRM_NSCHAIN_SET_OBJECT_DELETED,
                                                                   nschainset_object_handle,
                                                                   notification_data,
                                                                   app_entry_p->cbk_arg1,
                                                                   app_entry_p->cbk_arg2
                                                                   );
   }

   offset = NSRM_NSCHAINSET_OBJECT_TBL_OFFSET;
   MCHASH_DELETE_NODE_BY_REF(nsrm_nschainset_object_table_g, index, magic,
                             struct nsrm_nschainset_object *,offset, status_b);
   
   CNTLR_RCU_READ_LOCK_RELEASE();
   if(status_b == TRUE)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "nschainset object deleted successfully");
      return NSRM_SUCCESS;
   }
   else
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "failed to delete nschainset object");
      return NSRM_NSCHAINSET_OBJECT_DEL_FAIL;
   }
}


int32_t nsrm_modify_nschainset_object(
          struct  nsrm_nschainset_object_key*           key_info_p,
          int32_t number_of_config_params,
          struct  nsrm_nschainset_object_config_info*   config_info_p)
{
   struct nsrm_nschainset_object    *nschainset_object_node_scan_p = NULL;

   uint64_t   hashkey,offset;
   int32_t    ret_value , nschainset_object_node_found = 0;
   uint64_t   nschainset_object_handle;
   
   if(nsrm_nschainset_object_table_g == NULL)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR,"No hash table exists");
      return NSRM_FAILURE;
   }

   if(key_info_p->name_p == NULL || key_info_p->tenant_name_p == NULL)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR,"Invalid Key info");
      return NSRM_FAILURE;
   }

   ret_value = nsrm_get_nschainset_object_handle(key_info_p->name_p , &nschainset_object_handle);
   if(ret_value != NSRM_SUCCESS)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR,"Invalid nschainset object name");
      return NSRM_ERROR_NSCHAINSET_OBJECT_NAME_INVALID;
   }
   CNTLR_RCU_READ_LOCK_TAKE();
   hashkey = nsrm_get_hashval_byname(key_info_p->name_p,nsrm_no_of_nschainset_object_buckets_g);
   
   offset = NSRM_NSCHAINSET_OBJECT_TBL_OFFSET;
   MCHASH_BUCKET_SCAN(nsrm_nschainset_object_table_g ,hashkey ,nschainset_object_node_scan_p,
                      struct nsrm_nschainset_object* ,offset)
   {
      if((!strcmp(key_info_p->name_p , nschainset_object_node_scan_p-> name_p)) && 
         (!strcmp(key_info_p->tenant_name_p , nschainset_object_node_scan_p-> tenant_name_p)))
      {
         nschainset_object_node_found =1;
      }
   }
 
   if(!nschainset_object_node_found)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG,"nschainset name NOT found to modify %s" , key_info_p->name_p);
     CNTLR_RCU_READ_LOCK_RELEASE();
     return NSRM_NSCHAINSET_OBJECT_MOD_FAIL;
   }
   nschainset_object_node_scan_p->name_p = (char*)malloc(NSRM_MAX_NAME_LEN);
   nschainset_object_node_scan_p->tenant_name_p =(char*)malloc(NSRM_MAX_NAME_LEN);
   strcpy(nschainset_object_node_scan_p->name_p , key_info_p->name_p);
   strcpy(nschainset_object_node_scan_p->tenant_name_p , key_info_p->tenant_name_p);
   nschainset_object_node_scan_p->nschainset_type      = config_info_p[0].value.nschainset_type;
   nschainset_object_node_scan_p->admin_status_e       = config_info_p[1].value.admin_status_e;       

   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "nschainset Modified Successfully!");
   CNTLR_RCU_READ_LOCK_RELEASE();
   return NSRM_SUCCESS;
}
  
     
int32_t nsrm_get_first_nschainset_objects(
         int32_t  number_of_nschainset_objects_requested,
         int32_t* number_of_nschainset_objects_returned_p,
         struct   nsrm_nschainset_object_record* recs_p)
{
   struct nsrm_nschainset_object         *nschainset_object_node_scan_p = NULL;
   
   uint64_t   hashkey = 0;
   uint64_t   offset;
   uint32_t   nschainset_objects_returned = 0;
   uint32_t   nschainset_objects_requested = number_of_nschainset_objects_requested;
   int32_t    status = NSRM_FAILURE ;

   if(nsrm_nschainset_object_table_g == NULL)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR,"No hash table exists");
      return NSRM_FAILURE;
   }
 
   CNTLR_RCU_READ_LOCK_TAKE();
   offset = NSRM_NSCHAINSET_OBJECT_TBL_OFFSET;
   MCHASH_TABLE_SCAN(nsrm_nschainset_object_table_g , hashkey)
   {
      MCHASH_BUCKET_SCAN(nsrm_nschainset_object_table_g, hashkey, nschainset_object_node_scan_p,
                           struct nsrm_nschainset_object*,offset)
      {
         if((strlen(nschainset_object_node_scan_p->name_p) > NSRM_MAX_NAME_LENGTH) || (strlen(nschainset_object_node_scan_p->tenant_name_p) > NSRM_MAX_NAME_LENGTH))
           return NSRM_ERROR_LENGTH_NOT_ENOUGH;

         strcpy(recs_p->key.name_p , nschainset_object_node_scan_p->name_p);
         strcpy(recs_p->key.tenant_name_p , nschainset_object_node_scan_p->tenant_name_p);
        
         recs_p->info[0].value.nschainset_type   = nschainset_object_node_scan_p->nschainset_type;
         recs_p->info[1].value.admin_status_e = nschainset_object_node_scan_p->admin_status_e;
        
         status = NSRM_SUCCESS;
         number_of_nschainset_objects_requested--;
         nschainset_objects_returned++;
         
         if(number_of_nschainset_objects_requested)
           continue;
         else
           break;
      }
      if(number_of_nschainset_objects_requested)
        continue;
      else
        break;
   }
   
   if(nschainset_objects_requested != nschainset_objects_returned)
   {
     OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR,"Invalid number of objects returned / requested ");
     CNTLR_RCU_READ_LOCK_RELEASE();
     return NSRM_FAILURE; 
   }
   if(status == NSRM_SUCCESS)  
   { 
      *number_of_nschainset_objects_returned_p = nschainset_objects_returned;
       CNTLR_RCU_READ_LOCK_RELEASE();
       return status;
   }
}
      
  
int32_t nsrm_get_next_nschainset_object(
        struct   nsrm_nschainset_object_key* relative_record_key_p,
        int32_t  number_of_nschainset_objects_requested,
        int32_t* number_of_nschainset_objects_returned_p,
        struct   nsrm_nschainset_object_record* recs_p)
{
   struct nsrm_nschainset_object    *nschainset_object_node_scan_p = NULL;

   uint64_t  hashkey;
   uint64_t  offset ;
   uint8_t   current_entry_found_b;
   int32_t   nschainset_objects_returned = 0; 
   int32_t   nschainset_objects_requested = number_of_nschainset_objects_requested;
   int32_t   status = NSRM_FAILURE; 
  
   offset = NSRM_NSCHAINSET_OBJECT_TBL_OFFSET;
   CNTLR_RCU_READ_LOCK_TAKE();
   current_entry_found_b = FALSE;
     
   MCHASH_TABLE_SCAN(nsrm_nschainset_object_table_g,hashkey)
   {
      MCHASH_BUCKET_SCAN(nsrm_nschainset_object_table_g,hashkey,nschainset_object_node_scan_p,
                         struct nsrm_nschainset_object*, offset)
      {
         if(current_entry_found_b == FALSE)
         {
            if(((strcmp(relative_record_key_p->name_p , nschainset_object_node_scan_p->name_p)) != 0) ||
        ((strcmp(relative_record_key_p->tenant_name_p,nschainset_object_node_scan_p->tenant_name_p) !=0)))
                 continue; 
            else
            {
               OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "nschainset entry found copy next nschain");                       current_entry_found_b = TRUE;
               continue;
            }
         }
         strcpy(recs_p->key.name_p , nschainset_object_node_scan_p->name_p);
         strcpy(recs_p->key.tenant_name_p , nschainset_object_node_scan_p->tenant_name_p);
         recs_p->info[0].value.nschainset_type       = nschainset_object_node_scan_p->nschainset_type;
         recs_p->info[1].value.admin_status_e        = nschainset_object_node_scan_p->admin_status_e;
         status = NSRM_SUCCESS;
         number_of_nschainset_objects_requested--;
         nschainset_objects_returned++;
 
         if(number_of_nschainset_objects_requested)
           continue;
         else
           break;
      }
      if(number_of_nschainset_objects_requested)
        continue;
      else
        break;
   }
   if(nschainset_objects_requested != nschainset_objects_returned)
   {
     OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR,"Invalid number of objects returned / requested ");
     CNTLR_RCU_READ_LOCK_RELEASE();
     return NSRM_FAILURE; 
   }
   return status;
   if(status == NSRM_SUCCESS)
   {
      *number_of_nschainset_objects_returned_p = nschainset_objects_returned;
      CNTLR_RCU_READ_LOCK_RELEASE();
      return status;
   }
  
   CNTLR_RCU_READ_LOCK_RELEASE();
   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "Failed to copy nschainset object Info.....");

   if(current_entry_found_b == TRUE)
     return NSRM_ERROR_NO_MORE_ENTRIES;
   else
     return NSRM_ERROR_NSCHAINSET_OBJECT_NAME_INVALID;
}

  
int32_t nsrm_get_exact_nschainset_object(
        struct   nsrm_nschainset_object_key*   key_info_p,
        struct   nsrm_nschainset_object_record*   rec_p)
{
   struct nsrm_nschainset_object   *nschainset_object_node_scan_p = NULL;

   uint64_t hashkey,offset;
   uint8_t  current_entry_found_b = FALSE;
   
   if(nsrm_nschainset_object_table_g == NULL)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "nschainset object hash table doesnt exist");
      return NSRM_FAILURE;
   }     

  if(key_info_p->name_p == NULL || key_info_p->tenant_name_p == NULL)
  {
     OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "such node doesnt exist");
     return NSRM_FAILURE;
  }
  
   hashkey = nsrm_get_hashval_byname(key_info_p->name_p,nsrm_no_of_nschainset_object_buckets_g);
   offset = NSRM_NSCHAINSET_OBJECT_TBL_OFFSET;
   
   CNTLR_RCU_READ_LOCK_TAKE();
   MCHASH_BUCKET_SCAN(nsrm_nschainset_object_table_g, hashkey, nschainset_object_node_scan_p,
                      struct nsrm_nschainset_object*,offset)
   {  
      if((strcmp(key_info_p->name_p,nschainset_object_node_scan_p->name_p) == 0) && (strcmp(key_info_p->tenant_name_p,nschainset_object_node_scan_p->tenant_name_p) ==0 ))
      { 
         current_entry_found_b = TRUE;
      }   
      else   
        continue;
              
      strcpy(rec_p[0].key.name_p , nschainset_object_node_scan_p->name_p);
      strcpy(rec_p[0].key.tenant_name_p , nschainset_object_node_scan_p->tenant_name_p);
      rec_p[0].info[0].value.nschainset_type      = nschainset_object_node_scan_p->nschainset_type;
      rec_p[0].info[1].value.admin_status_e       = nschainset_object_node_scan_p->admin_status_e;

      CNTLR_RCU_READ_LOCK_RELEASE(); 
      return NSRM_SUCCESS;
   }
  CNTLR_RCU_READ_LOCK_RELEASE(); 
  OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "Failed to copy nschainset object Info.....");

  if(current_entry_found_b == TRUE)
    return NSRM_ERROR_NO_MORE_ENTRIES;
  else
    return NSRM_ERROR_NSCHAINSET_OBJECT_NAME_INVALID;
}

       
int32_t nsrm_register_nschainset_object_notifications(
                          uint8_t notification_type,
                          nsrm_nschainset_object_notifier  nschainset_notifier,
                          void* callback_arg1,
                          void* callback_arg2)    
{
   struct  nsrm_notification_app_hook_info *app_entry_p=NULL;
   uint8_t heap_b;
   int32_t retval = NSRM_SUCCESS;
   uchar8_t lstoffset;
   lstoffset = NSRM_NOTIFICATION_APP_HOOK_OFFSET;
  
   if((notification_type < NSRM_NSCHAINSET_OBJECT_FIRST_NOTIFICATION_TYPE) ||
     (notification_type > NSRM_NSCHAINSET_OBJECT_LAST_NOTIFICATION_TYPE))
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "Invalid Notification type:%d", notification_type);
      return NSRM_FAILURE;
   }
   if(nschainset_notifier == NULL)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "callback function is null");
      return NSRM_FAILURE;
   }

   retval = mempool_get_mem_block(nsrm_notifications_mempool_g, (uchar8_t**)&app_entry_p, &heap_b);
   if(retval != MEMPOOL_SUCCESS)
   {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_ERROR, "Failed to get mempool");
      return NSRM_FAILURE;
   }

   app_entry_p->call_back = (void*)nschainset_notifier;
   app_entry_p->cbk_arg1  = callback_arg1;
   app_entry_p->cbk_arg2  = callback_arg2;
   app_entry_p->heap_b    = heap_b;
  
   /* Add Application to the list of nschainset object notifications */
   OF_APPEND_NODE_TO_LIST(nsrm_nschainset_object_notifications[notification_type],app_entry_p, lstoffset); 
   if(retval != NSRM_SUCCESS)
     mempool_release_mem_block(nsrm_notifications_mempool_g,(uchar8_t*)app_entry_p, app_entry_p->heap_b);

   OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "callback function reg with nschainset object success");
   return retval;
}

int32_t nsrm_add_attribute_to_nschainset_object(
          char*     nschainset_object_name_p,
          struct    nsrm_attribute_name_value_pair *attribute_info_p)
{
  int32_t  ret_val;
  struct   nsrm_nschainset_object                         *nschainset_object_entry_p;
  union    nsrm_nschainset_object_notification_data        notification_data={};
  struct   nsrm_notification_app_hook_info               *app_entry_p;
  int32_t  apphookoffset;
  uint64_t nschainset_object_handle;
  apphookoffset = NSRM_NOTIFICATION_APP_HOOK_OFFSET;

  CNTLR_RCU_READ_LOCK_TAKE();
  ret_val = nsrm_get_nschainset_object_handle(nschainset_object_name_p,&nschainset_object_handle);
  if(ret_val != NSRM_SUCCESS)
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    return ret_val;
  }
  ret_val = nsrm_get_nschainset_byhandle(nschainset_object_handle,&nschainset_object_entry_p);
  if(ret_val != NSRM_SUCCESS)
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    return ret_val;
  }

  ret_val = nsrm_add_attribute(&(nschainset_object_entry_p->attributes),attribute_info_p);
  if(ret_val != NSRM_SUCCESS)
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    return ret_val;
  }
  OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "added attr");
  nschainset_object_entry_p->number_of_attributes++;
  /*
  of_create_crm_view_entry(nschainset_object_handle, attribute_info_p->attribute_name_p, attribute_info_p->attribute_value_p);
*/
  notification_data.attribute.nschainset_object_name_p = (char *)calloc(1,strlen(nschainset_object_entry_p->name_p)+1);
  notification_data.attribute.nschainset_attribute_name_p = (char *)calloc(1,strlen(attribute_info_p->attribute_name_p)+1);
  notification_data.attribute.nschainset_attribute_value_p = (char *)calloc(1,strlen(attribute_info_p->attribute_value_p)+1);
  strcpy(notification_data.attribute.nschainset_object_name_p, nschainset_object_entry_p->name_p);
  strcpy(notification_data.attribute.nschainset_attribute_name_p,attribute_info_p->attribute_name_p);
  strcpy(notification_data.attribute.nschainset_attribute_value_p,attribute_info_p->attribute_value_p);
  notification_data.attribute.nschainset_object_handle = nschainset_object_handle;

  OF_LIST_SCAN(nsrm_nschainset_object_notifications[NSRM_NSCHAIN_SET_OBJECT_ATTRIBUTE_ADDED],
               app_entry_p,
               struct nsrm_notification_app_hook_info *,
               apphookoffset)
  {
     ((nsrm_nschainset_object_notifier)(app_entry_p->call_back))(NSRM_NSCHAIN_SET_OBJECT_ATTRIBUTE_ADDED,
                                                                nschainset_object_handle,
                                                                notification_data,
                                                                app_entry_p->cbk_arg1,
                                                                 app_entry_p->cbk_arg2
                                                                   );
  }

   OF_LIST_SCAN(nsrm_nschainset_object_notifications[NSRM_NSCHAIN_SET_OBJECT_ALL] ,
                app_entry_p,
                struct nsrm_notification_app_hook_info *,
                apphookoffset)
   {
        ((nsrm_nschainset_object_notifier)(app_entry_p->call_back))(NSRM_NSCHAIN_SET_OBJECT_ATTRIBUTE_ADDED,
                                                                   nschainset_object_handle,
                                                                   notification_data,
                                                                   app_entry_p->cbk_arg1,
                                                                   app_entry_p->cbk_arg2
                                                                   );
   }

  OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "success");
  CNTLR_RCU_READ_LOCK_RELEASE();
  return NSRM_SUCCESS;
}
 


int32_t nsrm_del_attribute_from_nschainset_object(
                                                 char* nschainset_object_name_p,
                                                 char* attribute_name_p)
{
  int32_t  ret_val;
  struct   nsrm_nschainset_object                         *nschainset_object_entry_p;
  union    nsrm_nschainset_object_notification_data        notification_data={};
  struct   nsrm_notification_app_hook_info               *app_entry_p;
  struct   nsrm_resource_attribute_entry *attribute_entry_p,*prev_attr_entry_p = NULL;
  int32_t  apphookoffset;
  uint64_t nschainset_object_handle;
  apphookoffset = NSRM_NOTIFICATION_APP_HOOK_OFFSET;
  uint8_t  attr_node_found = 0;


  OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "Entered");
  CNTLR_RCU_READ_LOCK_TAKE();

  ret_val = nsrm_get_nschainset_object_handle(nschainset_object_name_p,&nschainset_object_handle);
  if(ret_val != NSRM_SUCCESS)
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    return ret_val;
  }
  ret_val = nsrm_get_nschainset_byhandle(nschainset_object_handle,&nschainset_object_entry_p);
  if(ret_val != NSRM_SUCCESS)
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    return ret_val;
  }
  
  OF_LIST_SCAN(nschainset_object_entry_p->attributes, attribute_entry_p, struct nsrm_resource_attribute_entry*,apphookoffset)
  {
    OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "attribute_entry_p->name:%s",attribute_entry_p->name);
    if(!strcmp(attribute_name_p, attribute_entry_p->name))
    {
      OF_LOG_MSG(OF_LOG_NSRM, OF_LOG_DEBUG, "attr name %s found to delete",attribute_entry_p->name);
      attr_node_found =1;
      break;
    }
    prev_attr_entry_p = attribute_entry_p;
  }

  if(attr_node_found == 0)
   {
     CNTLR_RCU_READ_LOCK_RELEASE();
     return NSRM_FAILURE;
   }
   notification_data.attribute.nschainset_object_name_p = (char *)calloc(1,strlen(nschainset_object_entry_p->name_p)+1);
  notification_data.attribute.nschainset_attribute_name_p = (char *)calloc(1,strlen(attribute_name_p)+1);
  /*
  notification_data.attribute.nschainset_attribute_value_p = (char *)calloc(1,strlen(attribute_info_p->attribute_value_p)+1);
  */
  strcpy(notification_data.attribute.nschainset_object_name_p, nschainset_object_entry_p->name_p);
  strcpy(notification_data.attribute.nschainset_attribute_name_p,attribute_name_p);
//  strcpy(notification_data.attribute.nschainset_attribute_value_p,attribute_info_p->attribute_value_p);
  notification_data.attribute.nschainset_object_handle = nschainset_object_handle;

  OF_LIST_SCAN(nsrm_nschainset_object_notifications[NSRM_NSCHAIN_SET_OBJECT_ATTRIBUTE_ADDED],
               app_entry_p,
               struct nsrm_notification_app_hook_info *,
               apphookoffset)
  {
     ((nsrm_nschainset_object_notifier)(app_entry_p->call_back))(NSRM_NSCHAIN_SET_OBJECT_ATTRIBUTE_ADDED,
                                                                nschainset_object_handle,
                                                                notification_data,
                                                                app_entry_p->cbk_arg1,
                                                                 app_entry_p->cbk_arg2
                                                                   );
  }

   OF_LIST_SCAN(nsrm_nschainset_object_notifications[NSRM_NSCHAIN_SET_OBJECT_ALL] ,
                app_entry_p,
                struct nsrm_notification_app_hook_info *,
                apphookoffset)
   {
        ((nsrm_nschainset_object_notifier)(app_entry_p->call_back))(NSRM_NSCHAIN_SET_OBJECT_ATTRIBUTE_ADDED,
                                                                   nschainset_object_handle,
                                                                   notification_data,
                                                                   app_entry_p->cbk_arg1,
                                                                   app_entry_p->cbk_arg2
                                                                   );
   }

   OF_REMOVE_NODE_FROM_LIST(nschainset_object_entry_p->attributes,attribute_entry_p,prev_attr_entry_p,apphookoffset);
   nschainset_object_entry_p->number_of_attributes--;
//   of_remove_crm_view_entry(attribute_entry_p->name, attribute_entry_p->value);

   CNTLR_RCU_READ_LOCK_RELEASE();
   return NSRM_SUCCESS;
}


int32_t nsrm_get_first_nschainset_object_attribute(
          char*   nschainset_object_name_p,
          struct  nsrm_attribute_name_value_output_info *attribute_output_p) 
{
  struct   nsrm_nschainset_object      *nschainset_object_entry_p;
  uint64_t nschainset_object_handle;
  int32_t ret_val = NSRM_SUCCESS;
  
  CNTLR_RCU_READ_LOCK_TAKE();

  do
  {
    ret_val = nsrm_get_nschainset_object_handle(nschainset_object_name_p,&nschainset_object_handle);
    if(ret_val != NSRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
    ret_val = nsrm_get_nschainset_byhandle(nschainset_object_handle,&nschainset_object_entry_p);
    if(ret_val != NSRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
   
    ret_val = nsrm_get_first_attribute(&(nschainset_object_entry_p->attributes), attribute_output_p);
    if(ret_val != NSRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
  }while(0);

  if(ret_val != NSRM_SUCCESS)
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    OF_LOG_MSG(OF_LOG_NSRM,OF_LOG_DEBUG,"Get first attribute failed");
    return ret_val;
  }
  else
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    OF_LOG_MSG(OF_LOG_NSRM,OF_LOG_DEBUG,"Get first attribute success");
    return ret_val;
  }
} 


int32_t nsrm_get_next_nschainset_object_attribute(
          char*   nschainset_object_name_p,
          char*   current_attribute_name_p,
          struct  nsrm_attribute_name_value_output_info *attribute_output_p) 
{
  struct   nsrm_nschainset_object      *nschainset_object_entry_p;
  uint64_t nschainset_object_handle;
  int32_t ret_val = NSRM_SUCCESS;
  
  CNTLR_RCU_READ_LOCK_TAKE();

  do
  {
    ret_val = nsrm_get_nschainset_object_handle(nschainset_object_name_p,&nschainset_object_handle);
    if(ret_val != NSRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
    ret_val = nsrm_get_nschainset_byhandle(nschainset_object_handle,&nschainset_object_entry_p);
    if(ret_val != NSRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
   
    ret_val = nsrm_get_next_attribute(&(nschainset_object_entry_p->attributes),current_attribute_name_p,
                                        attribute_output_p);
    if(ret_val != NSRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
  }while(0);

  if(ret_val != NSRM_SUCCESS)
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    OF_LOG_MSG(OF_LOG_NSRM,OF_LOG_DEBUG,"Get next attribute failed");
    return ret_val;
  }
  else
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    OF_LOG_MSG(OF_LOG_NSRM,OF_LOG_DEBUG,"Get next attribute success");
    return ret_val;
  }
} 


int32_t nsrm_get_exact_nschainset_object_attribute(
          char*  nschainset_object_name_p,
          char*  attribute_name_p,
          struct nsrm_attribute_name_value_output_info *attribute_output_p
          )
{
  struct   nsrm_nschainset_object      *nschainset_object_entry_p;
  uint64_t nschainset_object_handle;
  int32_t ret_val = NSRM_SUCCESS;
  
  CNTLR_RCU_READ_LOCK_TAKE();

  do
  {
    ret_val = nsrm_get_nschainset_object_handle(nschainset_object_name_p,&nschainset_object_handle);
    if(ret_val != NSRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
    ret_val = nsrm_get_nschainset_byhandle(nschainset_object_handle,&nschainset_object_entry_p);
    if(ret_val != NSRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
   
    ret_val = nsrm_get_exact_attribute(&(nschainset_object_entry_p->attributes),attribute_name_p,attribute_output_p);
    if(ret_val != NSRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
  }while(0);

  if(ret_val != NSRM_SUCCESS)
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    OF_LOG_MSG(OF_LOG_NSRM,OF_LOG_DEBUG,"Get exact attribute failed");
    return ret_val;
  }
  else
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    OF_LOG_MSG(OF_LOG_NSRM,OF_LOG_DEBUG,"Get exact attribute success");
    return ret_val;
  }
}  
 
   
   




