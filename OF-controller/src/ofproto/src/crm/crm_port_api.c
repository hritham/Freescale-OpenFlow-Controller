/** Cloud resource manager Virtual Network source file 
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

/* File  :      crm_port_api.c
 */

/*
 *
 * File name: crm_port_api.c
 * Author: Varun Kumar Reddy <B36461@freescale.com>
 * Date:   11/10/2013
 * Description: 
*/

/** Header files */
#include "controller.h"
#include "crmapi.h"
#include "crm_vn_api.h"
#include "crm_port_api.h"
#include "crm_vm_api.h"
#include "crm_tenant_api.h"
#include "cntrl_queue.h"
#include "cntlr_tls.h"
#include "cntlr_transprt.h"
#include "cntlr_event.h"
#include "crmldef.h"
#include "port_status_mgr.h"
#include "rrm_crm_rtlns_mgr.h"

/** Declarations */
extern void*     crm_port_mempool_g;
extern void*     crm_notifications_mempool_g;
extern uint32_t  crm_no_of_port_buckets_g;
extern struct    mchash_table* crm_port_table_g;

extern of_list_t crm_port_notifications[CRM_PORT_LAST_NOTIFICATION_TYPE + 1];

/** Definitions */
/***********************************************************************************************
* function:crm_add_crm_nwport
* description:
***********************************************************************************************/
int32_t crm_add_crm_nwport(struct crm_nwport_config_info *nw_port_info, uint64_t *output_crm_port_handle_p)
{
  uint64_t  switch_handle,bridge_handle;
  uchar8_t  heap_b;
  uchar8_t* hashobj_p = NULL;
  int32_t   status_b,ret_val = CRM_FAILURE;
  uint32_t  index,magic,apphookoffset,hashkey,offset;

  struct    crm_port*  crm_port_node_p      = NULL;
  struct    crm_port*  crm_port_node_scan_p = NULL;
  struct    crm_port_notification_data       notification_data={};
  struct    crm_notification_app_hook_info*  app_entry_p=NULL;  
  uint32_t  port_id,switch_ip;
  uint8_t   port_status;

  if((nw_port_info->port_name == NULL) ||
     (strlen(nw_port_info->port_name) >= CRM_MAX_PORT_NAME_LEN) ||
     (nw_port_info->switch_name == NULL) ||
     (nw_port_info->br_name     == NULL))
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_ERROR,"crm network port input config info invalid.");
    return CRM_FAILURE;
  }

  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"nw_port_info->port_name   =  %s",nw_port_info->port_name);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"nw_port_info->switch_name =  %s",nw_port_info->switch_name);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"nw_port_info->br_name     =  %s",nw_port_info->br_name);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"nw_port_info->port_type   =  %d",nw_port_info->port_type);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"nw_port_info->nw_type     =  %d",nw_port_info->nw_type);
 
  if(nw_port_info->nw_type == VXLAN_TYPE)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"service_port =  %x",(nw_port_info->nw_port_config_info).vxlan_info.service_port);
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"remote_ip    =  %x",(nw_port_info->nw_port_config_info).vxlan_info.remote_ip);  
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"vni          =  %x",(nw_port_info->nw_port_config_info).vxlan_info.nid);

    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"remoteip     =  %d",(nw_port_info->nw_port_config_info).vxlan_info.remote_ip);

    if((nw_port_info->nw_port_config_info).vxlan_info.remote_ip == 0)
      nw_port_info->port_type = NW_UNICAST_PORT;
    else
      nw_port_info->port_type = NW_BROADCAST_PORT;

  }
  else if (nw_port_info->nw_type == NVGRE_TYPE)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"service_port =  %x",(nw_port_info->nw_port_config_info).nvgre_info.service_port);
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"remote_ip    =  %x",(nw_port_info->nw_port_config_info).nvgre_info.remote_ip);
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"nid          =  %x",(nw_port_info->nw_port_config_info).nvgre_info.nid);

    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"remoteip     =  %d",(nw_port_info->nw_port_config_info).nvgre_info.remote_ip);

    if((nw_port_info->nw_port_config_info).nvgre_info.remote_ip == 0)
      nw_port_info->port_type = NW_UNICAST_PORT;
    else
      nw_port_info->port_type = NW_BROADCAST_PORT;
  }
  else
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_ERROR,"Invalid Network Type"); 
    return CRM_FAILURE;
  }  
  
  ret_val = rrm_check_crm_nwport_resource_relationships(nw_port_info->switch_name,nw_port_info->br_name,
                                                        &switch_handle,&bridge_handle,&switch_ip);
  if(ret_val != RRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"Incomplete Relationships");
    return CRM_ERROR_INCOMPLETE_RESOURCE_RELATIONSHIPS;
  }

  CNTLR_RCU_READ_LOCK_TAKE();  
  do
  {
    hashkey = crm_get_hashval_byname(nw_port_info->port_name, crm_no_of_port_buckets_g);
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "hashkey:%d\r\n",hashkey);
    offset = CRM_PORT_NODE_PORT_TBL_OFFSET;
    
    MCHASH_BUCKET_SCAN(crm_port_table_g, hashkey, crm_port_node_scan_p, struct crm_port*, offset)
    {
      if(strcmp(nw_port_info->port_name, crm_port_node_scan_p->port_name) != 0)
        continue;
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"duplicate crm port name %s", nw_port_info->port_name);
      ret_val = CRM_ERROR_DUPLICATE_CRM_PORT_RESOURCE;
      break;
    }
    /** create memory block for the crm port entry */
    ret_val = mempool_get_mem_block(crm_port_mempool_g, (uchar8_t **)&crm_port_node_p, &heap_b);
    if(ret_val != MEMPOOL_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"failed to allocate memory block for the crm port node.");
      ret_val = CRM_FAILURE;
      break;
    }
    crm_port_node_p->heap_b = heap_b;

    /** copy port config **/
    strcpy(crm_port_node_p->port_name, nw_port_info->port_name);
    strcpy(crm_port_node_p->switch_name, nw_port_info->switch_name);
    strcpy(crm_port_node_p->br1_name, nw_port_info->br_name);
 
    crm_port_node_p->switch_handle     = switch_handle;
    crm_port_node_p->crm_port_type     = nw_port_info->port_type;
    crm_port_node_p->system_br_saferef = bridge_handle;
    crm_port_node_p->nw_type           = nw_port_info->nw_type;
    crm_port_node_p->switch_ip         = switch_ip;   
 
    if(nw_port_info->nw_type == VXLAN_TYPE)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"nw_type is VXLAN"); 
      (crm_port_node_p->nw_params).vxlan_nw.nid         = (nw_port_info->nw_port_config_info).vxlan_info.nid;
       crm_port_node_p->nw_params.vxlan_nw.service_port = (nw_port_info->nw_port_config_info).vxlan_info.service_port;
       crm_port_node_p->nw_params.vxlan_nw.remote_ip    = (nw_port_info->nw_port_config_info).vxlan_info.remote_ip;
    }
    if(nw_port_info->nw_type == NVGRE_TYPE)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"nw_type is NVGRE");
      (crm_port_node_p->nw_params).nvgre_nw.nid         = (nw_port_info->nw_port_config_info).nvgre_info.nid;
       crm_port_node_p->nw_params.nvgre_nw.service_port = (nw_port_info->nw_port_config_info).nvgre_info.service_port;
       crm_port_node_p->nw_params.nvgre_nw.remote_ip    = (nw_port_info->nw_port_config_info).nvgre_info.remote_ip;
    }
    else if(nw_port_info->nw_type == VLAN_TYPE)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"port type is vlan"); 
      crm_port_node_p->nw_params.vlan_nw.vlan_id = (nw_port_info->nw_port_config_info).vlan_info.vlan_id;
    }

    crm_port_node_p->port_status = SYSTEM_PORT_NOT_READY; 
    crm_port_node_p->port_id     = 0;

    /* system port handle is not known at this time */
    
    ret_val = CRM_SUCCESS;
 
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"Appending crm /network port node");

    hashobj_p = (uchar8_t *)crm_port_node_p + CRM_PORT_NODE_PORT_TBL_OFFSET;
    MCHASH_APPEND_NODE(crm_port_table_g, hashkey, crm_port_node_p, index, magic, hashobj_p, status_b);
    if(status_b == FALSE) 
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"failed to append crm port  node to hash table\r\n", __FUNCTION__, __LINE__);
      ret_val = CRM_FAILURE;
      break;
    }
    *output_crm_port_handle_p = magic;
    *output_crm_port_handle_p = (((*output_crm_port_handle_p) <<32)|(index));

    crm_port_node_p->magic = magic;
    crm_port_node_p->index = index;
    crm_port_node_p->crm_port_handle = *output_crm_port_handle_p;
    OF_LIST_INIT(crm_port_node_p->attributes,crm_attributes_free_attribute_entry_rcu);
    
    apphookoffset = CRM_NOTIFICATION_APP_HOOK_OFFSET;

    /** fill the crm port notification data **/
    notification_data.new_port_id       =  crm_port_node_p->port_id;
    notification_data.crm_port_type     =  crm_port_node_p->crm_port_type;
    notification_data.crm_port_status   =  crm_port_node_p->port_status;
    strncpy(notification_data.port_name ,  crm_port_node_p->port_name, CRM_MAX_PORT_NAME_LEN);
    notification_data.crm_port_handle   =  crm_port_node_p->magic;
    notification_data.crm_port_handle   =  ((notification_data.crm_port_handle<<32)|(crm_port_node_p->index));
    notification_data.dp_handle         =  crm_port_node_p->system_br_saferef;
    strcpy(notification_data.swname , crm_port_node_p->switch_name);
    notification_data.switch_handle     =  crm_port_node_p->switch_handle;

    notification_data.nw_type = crm_port_node_p->nw_type;

    if(notification_data.nw_type == VXLAN_TYPE)
    {
      notification_data.nw_params.vxlan_nw.service_port = crm_port_node_p->nw_params.vxlan_nw.service_port; 
      notification_data.nw_params.vxlan_nw.remote_ip    = crm_port_node_p->nw_params.vxlan_nw.remote_ip;
      notification_data.nw_params.vxlan_nw.nid          = crm_port_node_p->nw_params.vxlan_nw.nid;
    }
    else if(notification_data.nw_type == NVGRE_TYPE)
    {
      notification_data.nw_params.nvgre_nw.service_port = crm_port_node_p->nw_params.nvgre_nw.service_port;
      notification_data.nw_params.nvgre_nw.remote_ip    = crm_port_node_p->nw_params.nvgre_nw.remote_ip;
      notification_data.nw_params.nvgre_nw.nid          = crm_port_node_p->nw_params.nvgre_nw.nid;
    }

    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "sending notifications");
    OF_LIST_SCAN(crm_port_notifications[CRM_PORT_STATUS], app_entry_p, struct crm_notification_app_hook_info *, apphookoffset )
    {
      ((crm_port_notifier_fn)(app_entry_p->call_back))(CRM_PORT_STATUS,
                              notification_data.crm_port_handle,
       		              notification_data,
                              app_entry_p->cbk_arg1,
                              app_entry_p->cbk_arg2
                             );
    }  
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"nw port added ");
  }while(0);
  
  CNTLR_RCU_READ_LOCK_RELEASE();
  if(ret_val != CRM_SUCCESS)
  {  
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "failed to add crm port to the .");

    if(crm_port_node_p)   
      mempool_release_mem_block(crm_port_mempool_g,(uchar8_t*)crm_port_node_p, crm_port_node_p->heap_b);
    return CRM_FAILURE;	
  }
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "crm nwport addition successful");
  return CRM_SUCCESS;
}
/*********************************************************************************************/
int32_t crm_get_first_nw_crm_port(char* port_name,
    struct crm_nwport_config_info *crm_nwport_info)
{
  struct crm_port *crm_port_node_p=NULL;
  uint32_t hashkey;
  uchar8_t offset;

  if(crm_port_table_g == NULL)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "no memory pool created for crm ports");
    return CRM_FAILURE;	
  }

  CNTLR_RCU_READ_LOCK_TAKE();
  
  offset =  CRM_PORT_NODE_PORT_TBL_OFFSET;
  MCHASH_TABLE_SCAN(crm_port_table_g,hashkey)
  {
    MCHASH_BUCKET_SCAN(crm_port_table_g,hashkey, crm_port_node_p,struct crm_port *,offset)
    {
      if((crm_port_node_p->crm_port_type==NW_UNICAST_PORT)||(crm_port_node_p->crm_port_type==NW_BROADCAST_PORT))
      {
        {
          strcpy(crm_nwport_info->port_name, crm_port_node_p->port_name);
          strcpy(port_name, crm_port_node_p->port_name);
          crm_nwport_info->port_type= crm_port_node_p->crm_port_type;
          crm_nwport_info->portId= crm_port_node_p->port_id;
          crm_nwport_info->nw_type= crm_port_node_p->nw_type;
          strcpy(crm_nwport_info->switch_name, crm_port_node_p->switch_name);
          strcpy(crm_nwport_info->br_name, crm_port_node_p->br1_name);
     
          if(crm_nwport_info->nw_type == VLAN_TYPE)
          {
            (crm_nwport_info->nw_port_config_info).vlan_info.vlan_id = (crm_port_node_p->nw_params).vlan_nw.vlan_id;
          }
          else if(crm_nwport_info->nw_type == VXLAN_TYPE)
          {
           (crm_nwport_info->nw_port_config_info).vxlan_info.service_port = (crm_port_node_p->nw_params).vxlan_nw.service_port;
           (crm_nwport_info->nw_port_config_info).vxlan_info.remote_ip= (crm_port_node_p->nw_params).vxlan_nw.remote_ip;
           (crm_nwport_info->nw_port_config_info).vxlan_info.nid= (crm_port_node_p->nw_params).vxlan_nw.nid;
          }
          else if(crm_nwport_info->nw_type == NVGRE_TYPE)
          {
           (crm_nwport_info->nw_port_config_info).nvgre_info.service_port = (crm_port_node_p->nw_params).nvgre_nw.service_port;
           (crm_nwport_info->nw_port_config_info).nvgre_info.remote_ip= (crm_port_node_p->nw_params).nvgre_nw.remote_ip;
           (crm_nwport_info->nw_port_config_info).nvgre_info.nid= (crm_port_node_p->nw_params).nvgre_nw.nid;
          }

          OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "port_name:%s,port_type:%d,br_name:%s,switch_name:%s,portId:%d,vlan_id:%d,vxid:%d,serviceport:%d,remoteip:%x",crm_port_node_p->port_name,crm_port_node_p->crm_port_type,crm_port_node_p->br1_name,crm_port_node_p->switch_name,crm_port_node_p->port_id,(crm_nwport_info->nw_port_config_info).vlan_info.vlan_id,(crm_nwport_info->nw_port_config_info).vxlan_info.nid,(crm_nwport_info->nw_port_config_info).vxlan_info.service_port,(crm_nwport_info->nw_port_config_info).vxlan_info.remote_ip);
          
          CNTLR_RCU_READ_LOCK_RELEASE();
          return CRM_SUCCESS;
        }
      }
    }
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_ERROR_NO_MORE_ENTRIES; 
}
/**********************************************************************************/
int32_t crm_get_next_nw_crm_port(char* port_name,
                                 struct crm_nwport_config_info *crm_nwport_info)
{
  struct crm_port *crm_port_node_p=NULL;
  uint32_t hashkey;
  uchar8_t offset;
  uint8_t current_entry_found_b=FALSE;


  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "entered");
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "port_name:%s",port_name);

  if(crm_port_table_g == NULL)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "no memory pool created for crm ports");
    return CRM_FAILURE;	
  }

  if((port_name==NULL))
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "names are NULL\n");
    return CRM_FAILURE;	
  }

  CNTLR_RCU_READ_LOCK_TAKE();
  MCHASH_TABLE_SCAN(crm_port_table_g,hashkey)
  {
    offset =  CRM_PORT_NODE_PORT_TBL_OFFSET;
    MCHASH_BUCKET_SCAN(crm_port_table_g,hashkey, crm_port_node_p, struct crm_port *,offset)
    {
      if(current_entry_found_b == FALSE)
      {
        if(strcmp(port_name,crm_port_node_p->port_name)!=0)
          continue;
        else
        {
          OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "port entry found .search for next nw port.");
          current_entry_found_b = TRUE;
          continue;
        }
      }
      else if((crm_port_node_p->crm_port_type==NW_UNICAST_PORT)||(crm_port_node_p->crm_port_type==NW_BROADCAST_PORT))
      {
        {
          strcpy(crm_nwport_info->port_name, crm_port_node_p->port_name);
          strcpy(port_name, crm_port_node_p->port_name);
          crm_nwport_info->port_type= crm_port_node_p->crm_port_type;
          crm_nwport_info->portId= crm_port_node_p->port_id;
          crm_nwport_info->nw_type= crm_port_node_p->nw_type;
          strcpy(crm_nwport_info->br_name, crm_port_node_p->br1_name);
          strcpy(crm_nwport_info->switch_name, crm_port_node_p->switch_name);

          if(crm_nwport_info->nw_type == VLAN_TYPE)
          {
            (crm_nwport_info->nw_port_config_info).vlan_info.vlan_id = (crm_port_node_p->nw_params).vlan_nw.vlan_id;
          }
          else if(crm_nwport_info->nw_type == VXLAN_TYPE)
          {
            (crm_nwport_info->nw_port_config_info).vxlan_info.service_port = (crm_port_node_p->nw_params).vxlan_nw.service_port;
            (crm_nwport_info->nw_port_config_info).vxlan_info.remote_ip= (crm_port_node_p->nw_params).vxlan_nw.remote_ip;
            (crm_nwport_info->nw_port_config_info).vxlan_info.nid= (crm_port_node_p->nw_params).vxlan_nw.nid;
          }
          else if(crm_nwport_info->nw_type == NVGRE_TYPE)
          {
            (crm_nwport_info->nw_port_config_info).nvgre_info.service_port = (crm_port_node_p->nw_params).nvgre_nw.service_port;
            (crm_nwport_info->nw_port_config_info).nvgre_info.remote_ip= (crm_port_node_p->nw_params).nvgre_nw.remote_ip;
            (crm_nwport_info->nw_port_config_info).nvgre_info.nid= (crm_port_node_p->nw_params).nvgre_nw.nid;
          }
 
          OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "port_name:%s,port_type:%d,br_name:%s,switch_name:%s,portId:%d,vlan_id:%d,vxid:%d,serviceport:%d,remoteip:%x",crm_port_node_p->port_name,crm_port_node_p->crm_port_type,crm_port_node_p->br1_name,crm_port_node_p->switch_name,crm_port_node_p->port_id,(crm_nwport_info->nw_port_config_info).vlan_info.vlan_id,(crm_nwport_info->nw_port_config_info).vxlan_info.nid,(crm_nwport_info->nw_port_config_info).vxlan_info.service_port,(crm_nwport_info->nw_port_config_info).vxlan_info.remote_ip);

          
          CNTLR_RCU_READ_LOCK_RELEASE();
          return CRM_SUCCESS;
        }
      }      
    }
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_ERROR_NO_MORE_ENTRIES; 
}
/*******************************************************************/
int32_t crm_get_exact_nwport(uint64_t handle,
	                     struct  crm_nwport_config_info *crm_nwport_info_p)
{
  uint32_t index,magic;
  struct   crm_port *crm_port_node_p;
  uint8_t  status_b;

  magic = (uint32_t)(handle >>32);
  index = (uint32_t)handle;

  CNTLR_RCU_READ_LOCK_TAKE();

  MCHASH_ACCESS_NODE(crm_port_table_g,index,magic,crm_port_node_p,status_b);
  if(TRUE == status_b)
  { 
    if((crm_port_node_p->crm_port_type==NW_UNICAST_PORT)||(crm_port_node_p->crm_port_type==NW_BROADCAST_PORT))
    {
      {
        strcpy(crm_nwport_info_p->port_name, crm_port_node_p->port_name);
        crm_nwport_info_p->port_type= crm_port_node_p->crm_port_type;
        crm_nwport_info_p->portId= crm_port_node_p->port_id;
        crm_nwport_info_p->nw_type= crm_port_node_p->nw_type;
        strcpy(crm_nwport_info_p->br_name, crm_port_node_p->br1_name);
        if(crm_nwport_info_p->nw_type == VLAN_TYPE)
        {
          (crm_nwport_info_p->nw_port_config_info).vlan_info.vlan_id = (crm_port_node_p->nw_params).vlan_nw.vlan_id;
        }
        else if(crm_nwport_info_p->nw_type == VXLAN_TYPE)
        {
          (crm_nwport_info_p->nw_port_config_info).vxlan_info.service_port = (crm_port_node_p->nw_params).vxlan_nw.service_port;
          (crm_nwport_info_p->nw_port_config_info).vxlan_info.remote_ip= (crm_port_node_p->nw_params).vxlan_nw.remote_ip;
          (crm_nwport_info_p->nw_port_config_info).vxlan_info.nid= (crm_port_node_p->nw_params).vxlan_nw.nid;
        }
        else if(crm_nwport_info_p->nw_type == NVGRE_TYPE)
        {
          (crm_nwport_info_p->nw_port_config_info).nvgre_info.service_port = (crm_port_node_p->nw_params).nvgre_nw.service_port;
          (crm_nwport_info_p->nw_port_config_info).nvgre_info.remote_ip= (crm_port_node_p->nw_params).nvgre_nw.remote_ip;
          (crm_nwport_info_p->nw_port_config_info).nvgre_info.nid= (crm_port_node_p->nw_params).nvgre_nw.nid;
        }


        OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "port_name:%s,port_type:%d,br_name:%s,switch_name:%s,portId:%d",crm_port_node_p->port_name,crm_port_node_p->crm_port_type,crm_port_node_p->br1_name,crm_port_node_p->switch_name,crm_port_node_p->port_id);

        CNTLR_RCU_READ_LOCK_RELEASE();
        return CRM_SUCCESS;
      }
    }
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_ERROR_INVALID_SWITCH_HANDLE;
}
/***************************************************************************/
int32_t crm_del_crm_nwport(char* nwport_name_p)
{
  int32_t  ret_val;
  int32_t  nwport_node_found=0;
  uint32_t offset, hashkey,magic,index;
  uint8_t  status_b;
  struct   crm_port *crm_nwport_scan_p=NULL;
  struct   crm_port_notification_data  notification_data={};
  struct   crm_notification_app_hook_info *app_entry_p=NULL;

  uint32_t apphookoffset; 

  if(nwport_name_p == NULL)
    return CRM_ERROR_NWPORT_NAME_NULL;

  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "nwport_name_p %s",nwport_name_p);

  if(ret_val != RRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"Incomplete Relationships");
    return CRM_ERROR_INCOMPLETE_RESOURCE_RELATIONSHIPS;
  }

  do
  {
    /** calculate hash key for the virtual network name */
    hashkey = crm_get_hashval_byname(nwport_name_p, crm_no_of_port_buckets_g);
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "hashkey:%d\r\n",hashkey);

    offset =  CRM_PORT_NODE_PORT_TBL_OFFSET;

    CNTLR_RCU_READ_LOCK_TAKE();
    MCHASH_BUCKET_SCAN(crm_port_table_g, hashkey, crm_nwport_scan_p, struct crm_port*, offset)
    {
      if(!strcmp(nwport_name_p, crm_nwport_scan_p->port_name))
      {
        if((crm_nwport_scan_p->crm_port_type  != NW_UNICAST_PORT)&&
            (crm_nwport_scan_p->crm_port_type != NW_BROADCAST_PORT))
         {
           OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "crm network port type is not unicast or broadcast");
           ret_val = CRM_FAILURE;
           break;
         }
       
         OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "crm network port found to delete");
         nwport_node_found = 1;
         magic  = (crm_nwport_scan_p->magic);
         index  = (crm_nwport_scan_p->index);
         break;
      }
    }
    if(nwport_node_found == 0)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "network port node not found to delete");
      ret_val= CRM_FAILURE;
      break;	
    }

    /* fill the notification data */

    strncpy(notification_data.port_name , crm_nwport_scan_p->port_name, CRM_MAX_PORT_NAME_LEN);

    notification_data.new_port_id       =  crm_nwport_scan_p->port_id;
    notification_data.crm_port_type     =  crm_nwport_scan_p->crm_port_type;
    notification_data.crm_port_status   =  crm_nwport_scan_p->port_status;
    notification_data.crm_port_handle   =  magic;
    notification_data.crm_port_handle   =  ((notification_data.crm_port_handle<<32)|(index));
    notification_data.dp_handle         =  crm_nwport_scan_p->system_br_saferef;

    OF_LIST_SCAN(crm_port_notifications[CRM_PORT_DELETE], app_entry_p, struct crm_notification_app_hook_info *, apphookoffset )
    {
      ((crm_port_notifier_fn)(app_entry_p->call_back))(CRM_PORT_DELETE,
						       notification_data.crm_port_handle,
    				                       notification_data,
     						       app_entry_p->cbk_arg1,
 						       app_entry_p->cbk_arg2);
    }

    OF_LIST_SCAN(crm_port_notifications[CRM_VN_ALL_NOTIFICATIONS], app_entry_p, struct crm_notification_app_hook_info *, apphookoffset )
    {
      ((crm_port_notifier_fn)(app_entry_p->call_back))(CRM_PORT_DELETE,
      					 	       notification_data.crm_port_handle,
      						       notification_data,
      						       app_entry_p->cbk_arg1,
      						       app_entry_p->cbk_arg2);
    }

    // No resource relationships are maintained for network pors */

    status_b = FALSE;
    offset   = CRM_PORT_NODE_PORT_TBL_OFFSET;

    MCHASH_DELETE_NODE_BY_REF(crm_port_table_g,index,magic,struct crm_port*, offset, status_b);
    if(status_b == TRUE)
      ret_val = CRM_SUCCESS;
    else
      ret_val = CRM_FAILURE;        

  }while(0);

  CNTLR_RCU_READ_LOCK_RELEASE();
  return ret_val;  
}
/***************************************************************************/
crm_add_guest_crm_vmport(uint8_t*  mac_p,uint32_t ipaddr,
                         uint64_t*  output_crm_port_handle_p)
{
  uint8_t  port_type = VM_GUEST_PORT;
  char     port_name[16];
  struct   crm_port* crm_port_node_p;
  struct   crm_port* crm_port_node_scan_p=NULL;
  uint8_t  heap_b;

  uchar8_t* hashobj_p = NULL;
  int32_t   status_b,ret_val=CRM_SUCCESS;
  uint32_t  index,magic,hashkey,offset;

  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"generate a new guest port name");
  
  strcpy(port_name,"guestport");

  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"guest port name = %s",port_name);

  /** calculate hash key for the port name */
  hashkey = crm_get_hashval_byname(port_name, crm_no_of_port_buckets_g);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "hashkey:%d\r\n",hashkey);

  offset = CRM_PORT_NODE_PORT_TBL_OFFSET;
  
  CNTLR_RCU_READ_LOCK_TAKE();
  do
  {
    /** scan crm port hash table for the port name */
    MCHASH_BUCKET_SCAN(crm_port_table_g, hashkey, crm_port_node_scan_p, struct crm_port*, offset)
    {
      if(strcmp(port_name, crm_port_node_scan_p->port_name) != 0)
        continue;
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"duplicate crm port name %s", port_name);
      ret_val = CRM_ERROR_DUPLICATE_CRM_PORT_RESOURCE;
      break;
    }

    /** create memory block for the crm port entry */
    ret_val = mempool_get_mem_block(crm_port_mempool_g, (uchar8_t **)&crm_port_node_p, &heap_b);
    if(ret_val != MEMPOOL_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"failed to allocate memory block for the crm port node.");
      ret_val = CRM_FAILURE;
      break;
    }
    crm_port_node_p->heap_b = heap_b;
 
    /** copy port config **/
    strcpy(crm_port_node_p->port_name, port_name);
    crm_port_node_p->crm_port_type = port_type;
    crm_port_node_p->switch_ip = ipaddr;
    memcpy(crm_port_node_p->vmInfo.vm_port_mac, mac_p,6);
 
    /** append crm port to hash table **/
    hashobj_p = (uchar8_t *)crm_port_node_p + CRM_PORT_NODE_PORT_TBL_OFFSET;
    MCHASH_APPEND_NODE(crm_port_table_g, hashkey, crm_port_node_p, index, magic, hashobj_p, status_b);
    if(FALSE == status_b)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"failed to append crm port  node to hash table\r\n", __FUNCTION__, __LINE__);
      ret_val = CRM_FAILURE;
      break;
    }
    *output_crm_port_handle_p = magic;
    *output_crm_port_handle_p = (((*output_crm_port_handle_p) <<32)|(index));

    crm_port_node_p->magic = magic;
    crm_port_node_p->index = index;
    crm_port_node_p->crm_port_handle = *output_crm_port_handle_p;
  }while(0);

  if(ret_val != CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "failed to add VM_GUEST_PORT to the .crm_port_node_p%x",crm_port_node_p);

    if(crm_port_node_p)
      mempool_release_mem_block(crm_port_mempool_g,(uchar8_t*)crm_port_node_p, crm_port_node_p->heap_b);

    CNTLR_RCU_READ_LOCK_RELEASE();

    return CRM_FAILURE;
  }
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "crm vm port addition successfull");
  CNTLR_RCU_READ_LOCK_RELEASE();
  
  return CRM_SUCCESS;
}
/***************************************************************************
* function: crm_add_crm_vmport_to_vn
* description:
***************************************************************************/
int32_t crm_add_crm_vmport(char*      port_name,
		           char*      switch_name,
                           char*      br_name,
                           int32_t    port_type,
                           char*      vn_name,
                           char*      vm_name,
                           uint8_t*   vm_port_mac_p,
                           uint64_t*  output_crm_port_handle_p)
{
  uint64_t  switch_handle,vn_handle,vm_handle,logical_switch_handle;
  uchar8_t  heap_b;
  uchar8_t* hashobj_p = NULL;
  int32_t   status_b,ret_val=CRM_FAILURE;
  uint32_t  index,magic,apphookoffset,hashkey,offset;

  struct    crm_port                        *crm_port_node_p = NULL, *crm_port_node_scan_p = NULL;
  struct    crm_port_notification_data      notification_data = {};
  struct    crm_notification_app_hook_info  *app_entry_p = NULL;

  uint32_t port_id,switch_ip;
  uint8_t  nw_type,port_status;

  if((port_name == NULL) || (strlen(port_name)>= CRM_MAX_PORT_NAME_LEN)||
     (vn_name == NULL)   || (switch_name == NULL) || (br_name == NULL))
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_ERROR,"Invalid input parameters for crm port add");
    return CRM_FAILURE;
  }

  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"port_name   = %s",port_name);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"switch_name = %s",switch_name);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"br_name     = %s",br_name);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"port_type   = %d",port_type);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"vn_name     = %s",vn_name);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"vm_name     = %s",vm_name);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"port_mac    = %x",vm_port_mac_p[5]);

  do
  {
    if(!((port_type == VMNS_PORT)||(port_type == VM_PORT)))    	
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_ERROR, "Invalid CRM Port Type");
      ret_val = CRM_FAILURE;
      break;	
    }

    ret_val = rrm_check_crmport_resource_relationships(switch_name,br_name,
                                                       vn_name, vm_name,
                                                       &switch_handle,&logical_switch_handle,
                                                       &vn_handle,&vm_handle,
                                                       &switch_ip,&nw_type);
    if(ret_val != RRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"Incomplete Relationships");
      ret_val= CRM_ERROR_INCOMPLETE_RESOURCE_RELATIONSHIPS;
      break;
    }

    hashkey = crm_get_hashval_byname(port_name, crm_no_of_port_buckets_g);
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "hashkey = %d",hashkey);

    offset = CRM_PORT_NODE_PORT_TBL_OFFSET;

    /* scan crm port hash table for a duplicate port name */
    MCHASH_BUCKET_SCAN(crm_port_table_g, hashkey, crm_port_node_scan_p, struct crm_port*, offset)
    {
      if(strcmp(port_name, crm_port_node_scan_p->port_name) != 0)
        continue;
  
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"Duplicate crm port name %s", port_name);
      ret_val = CRM_ERROR_DUPLICATE_CRM_PORT_RESOURCE;
      break;
    }

    /* create memory block for the crm port entry */
    ret_val = mempool_get_mem_block(crm_port_mempool_g, (uchar8_t **)&crm_port_node_p, &heap_b);
    if(ret_val != MEMPOOL_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"failed to allocate memory block for the crm port node.");
      ret_val = CRM_FAILURE;
      break;
    }

    crm_port_node_p->heap_b = heap_b;

    /* copy port config */
    strcpy(crm_port_node_p->vn_name,vn_name);
    strcpy(crm_port_node_p->br1_name,br_name);
    strcpy(crm_port_node_p->port_name,port_name);
    strcpy(crm_port_node_p->switch_name,switch_name);
    strcpy(crm_port_node_p->vmInfo.vm_name,vm_name);

    crm_port_node_p->crm_port_type = port_type;
    crm_port_node_p->switch_handle = switch_handle;
    crm_port_node_p->saferef_vn    = vn_handle;
    crm_port_node_p->system_br_saferef  = logical_switch_handle;
    crm_port_node_p->vmInfo.saferef_vm  = vm_handle;
    crm_port_node_p->switch_ip          = switch_ip;
    crm_port_node_p->nw_type            = nw_type;
   
    memcpy(crm_port_node_p->vmInfo.vm_port_mac, vm_port_mac_p, sizeof(crm_port_node_p->vmInfo.vm_port_mac));

    port_status = SYSTEM_PORT_NOT_READY; 
    crm_port_node_p->port_id = 0;   

    /* Append crm port to hash table anyway */
    hashobj_p = (uchar8_t *)crm_port_node_p + CRM_PORT_NODE_PORT_TBL_OFFSET;
    MCHASH_APPEND_NODE(crm_port_table_g, hashkey, crm_port_node_p, index, magic, hashobj_p, status_b);
    if(FALSE == status_b)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"failed to append crm port  node to hash table\r\n", __FUNCTION__, __LINE__);
      ret_val = CRM_FAILURE;
      break;
    }
    *output_crm_port_handle_p = magic;
    *output_crm_port_handle_p = (((*output_crm_port_handle_p) <<32)|(index));
    crm_port_node_p->magic    = magic;
    crm_port_node_p->index    = index;
    crm_port_node_p->crm_port_handle = *output_crm_port_handle_p;

    OF_LIST_INIT(crm_port_node_p->attributes,crm_attributes_free_attribute_entry_rcu);
    ret_val = rrm_setup_crm_vmport_resource_relationships(port_name,crm_port_node_p->crm_port_handle,
                                                          crm_port_node_p->crm_port_type,
                                                          vm_name,vn_name,
                                                          switch_name,br_name); 
    if(ret_val != RRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_ERROR,"Resource Relationship Manager returned error");   
      break; 
    }

    /* Send Notification if port is active */
    apphookoffset = CRM_NOTIFICATION_APP_HOOK_OFFSET;
    /** fill the notification data **/
    strcpy(notification_data.vn_name,vn_name);
    notification_data.new_port_id       =  crm_port_node_p->port_id;
    notification_data.crm_port_type     =  crm_port_node_p->crm_port_type;
    notification_data.crm_port_status   =  crm_port_node_p->port_status;
    strcpy(notification_data.port_name , crm_port_node_p->port_name);
    strcpy(notification_data.swname    , crm_port_node_p->switch_name);
    notification_data.crm_port_handle   =  crm_port_node_p->magic;
    notification_data.crm_port_handle   =  ((notification_data.crm_port_handle<<32)|(crm_port_node_p->index));
    notification_data.crm_vn_handle     =  crm_port_node_p->saferef_vn;
    notification_data.dp_handle         =  crm_port_node_p->system_br_saferef;
    notification_data.crm_vm_handle     =  crm_port_node_p->vmInfo.saferef_vm;
    /* System port handle is not known at this time */

    OF_LIST_SCAN(crm_port_notifications[CRM_PORT_STATUS], app_entry_p, struct crm_notification_app_hook_info *, apphookoffset)
    {
      ((crm_port_notifier_fn)(app_entry_p->call_back))(CRM_PORT_STATUS,
    						       notification_data.crm_port_handle,
        					       notification_data,
        					       app_entry_p->cbk_arg1,
        					       app_entry_p->cbk_arg2);
    }  
  }while(0);

  if(ret_val != CRM_SUCCESS)
  {  
    if(crm_port_node_p)   
      mempool_release_mem_block(crm_port_mempool_g,(uchar8_t*)crm_port_node_p, crm_port_node_p->heap_b);

    CNTLR_RCU_READ_LOCK_RELEASE();
    return CRM_FAILURE;
  }

  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "CRM VM Port Addition Successful");
  CNTLR_RCU_READ_LOCK_RELEASE();
  
  return CRM_SUCCESS;
}
/******************************************************************************/
int32_t crm_get_first_crm_vmport(char* vn_name, char* switch_name,
 				 char* logical_switch_name, char* port_name,
				 struct crm_port_config_info *crm_port_info)
{
  struct crm_port *crm_port_node_p=NULL;
  uint32_t hashkey;
  uchar8_t offset;

  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "vn_name:%s,switch_name:%s",vn_name,switch_name);

  if(crm_port_table_g == NULL)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "no memory pool created for crm ports");
    return CRM_FAILURE;	
  }

  if((vn_name==NULL)|| (switch_name == NULL))
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "names are NULL\n");
    return CRM_FAILURE;	
  }

  CNTLR_RCU_READ_LOCK_TAKE();
  offset =  CRM_PORT_NODE_PORT_TBL_OFFSET;
  MCHASH_TABLE_SCAN(crm_port_table_g,hashkey)
  {
    MCHASH_BUCKET_SCAN(crm_port_table_g,hashkey, crm_port_node_p,struct crm_port *,offset)
    {
      if((crm_port_node_p->crm_port_type==VM_PORT)||(crm_port_node_p->crm_port_type==VMNS_PORT))
      {
        OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "entered");
        if((!strcmp(vn_name,crm_port_node_p->vn_name))&&(!strcmp(switch_name,crm_port_node_p->switch_name)))
        {
          OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "entered");
          strcpy(crm_port_info->port_name, crm_port_node_p->port_name);
          strcpy(port_name, crm_port_node_p->port_name);
          crm_port_info->port_type= crm_port_node_p->crm_port_type;
          crm_port_info->portId= crm_port_node_p->port_id;
          strcpy(crm_port_info->vm_name, crm_port_node_p->vmInfo.vm_name);
          memcpy(crm_port_info->vm_port_mac_p, crm_port_node_p->vmInfo.vm_port_mac, sizeof(crm_port_node_p->vmInfo.vm_port_mac));
          strcpy(crm_port_info->br_name, crm_port_node_p->br1_name);
          OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "port_name:%s,port_type:%d,vm_name:%s,max:%s,br_name:%s",crm_port_node_p->port_name,crm_port_node_p->crm_port_type,crm_port_node_p->vmInfo.vm_name,crm_port_node_p->vmInfo.vm_port_mac,crm_port_node_p->br1_name);

           CNTLR_RCU_READ_LOCK_RELEASE();
           return CRM_SUCCESS;
        }
      }
    }
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_ERROR_NO_MORE_ENTRIES;
}
/****************************************************************************/
int32_t crm_get_next_crm_vmport(char* vn_name, char* switch_name,
				char* logical_switch_name, char* port_name,
    				struct crm_port_config_info *crm_port_info)
{
  struct crm_port *crm_port_node_p=NULL;
  uint32_t hashkey;
  uchar8_t offset;
  uint8_t current_entry_found_b=FALSE;

  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "vn_name:%s,switch_name:%s,logical_switch_name:%s,port_name:%s",vn_name,switch_name,logical_switch_name,port_name);

  if(crm_port_table_g == NULL)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "no memory pool created for crm ports");
    return CRM_FAILURE;	
  }

  if((vn_name==NULL) || (switch_name == NULL) || (logical_switch_name == NULL))
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "names are NULL\n");
    return CRM_FAILURE;	
  }

  CNTLR_RCU_READ_LOCK_TAKE();
  MCHASH_TABLE_SCAN(crm_port_table_g,hashkey)
  {
    offset =  CRM_PORT_NODE_PORT_TBL_OFFSET;
    MCHASH_BUCKET_SCAN(crm_port_table_g,hashkey, crm_port_node_p, struct crm_port *,offset)
    {
      if(current_entry_found_b == FALSE)
      {
        if(strcmp(port_name,crm_port_node_p->port_name)!=0)
          continue;
        else
        {
          OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "port entry found .search for next vmorvmns port.");
          current_entry_found_b = TRUE;
          continue;
        }
      }
      else if((crm_port_node_p->crm_port_type==VM_PORT)||((crm_port_node_p->crm_port_type==VMNS_PORT)))
      {
        if((!strcmp(vn_name,crm_port_node_p->vn_name))&&(!strcmp(switch_name,crm_port_node_p->switch_name)))
        {
          strcpy(crm_port_info->port_name, crm_port_node_p->port_name);
          crm_port_info->port_type= crm_port_node_p->crm_port_type;
          crm_port_info->portId= crm_port_node_p->port_id;
          strcpy(crm_port_info->vm_name, crm_port_node_p->vmInfo.vm_name);
          memcpy(crm_port_info->vm_port_mac_p, crm_port_node_p->vmInfo.vm_port_mac, sizeof(crm_port_node_p->vmInfo.vm_port_mac));
          strcpy(crm_port_info->br_name, crm_port_node_p->br1_name);
          OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "port_name:%s,port_type:%d,vm_name:%s,max:%s,br_name:%s",crm_port_node_p->port_name,crm_port_node_p->crm_port_type,crm_port_node_p->vmInfo.vm_name,crm_port_node_p->vmInfo.vm_port_mac,crm_port_node_p->br1_name);
          
        
          CNTLR_RCU_READ_LOCK_RELEASE();
          return CRM_SUCCESS;
        }
      }
    }
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_ERROR_NO_MORE_ENTRIES;
}
/***************************************************************************/
int32_t crm_get_exact_crm_vmport(char* vn_name, char* switch_name,
				 char* logical_switch_name, char* port_name,
    				 struct crm_port_config_info *crm_port_info)

{
  uint64_t  vn_handle;
  int32_t   ret_val = CRM_FAILURE;
  uint32_t  crm_compute_node_found = FALSE,index = 0;
  uint32_t  port_node_found = FALSE;

  struct  crm_virtual_network*	crm_vn_info_p=NULL;
  struct  crm_compute_node*     crm_compute_node_scan_entry_p=NULL;
  struct  crm_port_ref*         crm_logical_switch_ports_node_p=NULL;

  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "vn_name:%s,switch_name:%s,logical_switch_name:%s",vn_name,switch_name,logical_switch_name);

  if((vn_name==NULL)|| (switch_name == NULL))
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,  "names are NULL\n");
    return CRM_FAILURE;	
  }

  /* get the vn handle and vn info */
  ret_val = crm_get_vn_handle(vn_name, &vn_handle);
  if(ret_val != CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "virtual network  name invalid!.");
    return CRM_ERROR_VN_NAME_INVALID;
  }

  ret_val = crm_get_vn_byhandle(vn_handle, &crm_vn_info_p);
  if(ret_val != CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "get virtual network info failed!.");
    return CRM_ERROR_INVALID_VN_HANDLE;
  }

  CNTLR_RCU_READ_LOCK_TAKE();  
  do
  {
    /* check the compute node in  vn db */
    OF_LIST_SCAN(crm_vn_info_p->crm_compute_nodes, 
  	         crm_compute_node_scan_entry_p, 
                 struct crm_compute_node*, 
                 CRM_VN_COMPUTE_NODE_ENTRY_LISTNODE_OFFSET
               )
    {
      if(!strcmp(crm_compute_node_scan_entry_p->switch_name, switch_name))
      {
        OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "crm compute node  found");
        crm_compute_node_found = 1;
        break;
      }
    }
    if(!crm_compute_node_found)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "crm compute node not found!.");
      ret_val= CRM_FAILURE;
      break;
    }

    ret_val = CRM_ERROR_NO_MORE_ENTRIES;
    if(crm_compute_node_scan_entry_p->no_of_bridges == 0)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "no bridges attached to computenode!.");
      break;
    }
   
    for(index=0; index<(crm_compute_node_scan_entry_p->no_of_bridges); index++)
    {
      OF_LIST_SCAN(((crm_compute_node_scan_entry_p->dp_references[index]).vmside_ports),
   	             crm_logical_switch_ports_node_p,
                     struct crm_port_ref *,
                     CRM_LOGICAL_SWITCH_PORT_NODE_OFFSET)
      {
        if(!strcmp(crm_logical_switch_ports_node_p->port_name,port_name))
        {
          OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "crm port found!");
          port_node_found =TRUE;
          break;
        }     
      }
      if(port_node_found == TRUE)break;
    }
    if(port_node_found == FALSE)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "crm port not found!");
      for(index=0; index<(crm_compute_node_scan_entry_p->no_of_bridges); index++)
      {
        OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "crm port searching in vmns!");
     
        OF_LIST_SCAN(((crm_compute_node_scan_entry_p->dp_references[index]).vmns_ports),
     	               crm_logical_switch_ports_node_p,
                       struct crm_port_ref*,
                       CRM_LOGICAL_SWITCH_PORT_NODE_OFFSET)
        {
          OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "crm port found in vmns!");
    
          if(!strcmp(crm_logical_switch_ports_node_p->port_name,port_name))
          {
            OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "crm port found!");
            port_node_found =TRUE;
            break;
          }     
        }
        if(port_node_found == TRUE)break;
      }
    }

    if(port_node_found == FALSE)
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "crm port entry not found!.");
      ret_val= CRM_FAILURE;
      break;
    }

    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "return success");
    ret_val=CRM_SUCCESS;

  }while(0);

  CNTLR_RCU_READ_LOCK_RELEASE();
  return ret_val; 
}
/***************************************************************************
function : crm_del_crm_port_from_vn
description:
this function detaches the port from the given vn,sw,br 
***************************************************************************/
int32_t crm_del_crm_vmport(char*     port_name,
	                   int32_t   port_type,
    			   char*     vn_name,
    			   char*     switch_name,
    			   char*     vm_name,
    			   char*     br_name)
{
  uint64_t vn_handle;
  uint32_t apphookoffset,offset; 
  uint64_t switch_handle,logical_switch_handle,vm_handle,port_handle;
  struct   crm_virtual_network* crm_vn_info_p = NULL;
  struct   crm_port* crm_port_node_scan_p = NULL;
  struct   crm_port_notification_data notification_data = {};
  struct   crm_notification_app_hook_info* app_entry_p = NULL;
  uint32_t switch_ip; 
  uint8_t  nw_type;

  int32_t  ret_val = CRM_FAILURE;
  int32_t  status_b;
  
  if((port_name == NULL)|| (vn_name==NULL)||(switch_name == NULL) || (br_name == NULL))
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"invalid crm ports data supplied to delete.");
    return CRM_FAILURE;
  }

  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"port_name   = %s",port_name);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"switch_name = %s",switch_name);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"br_name     = %s",br_name);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"port_type   = %d",port_type);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"vn_name     = %s",vn_name);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"vm_name     = %s",vm_name);

  ret_val = crm_get_port_byname(port_name, &crm_port_node_scan_p);
  if(ret_val != CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "port name invalid");
    return CRM_FAILURE;
  }

  port_type = crm_port_node_scan_p->crm_port_type;
  rrm_check_crmport_resource_relationships(switch_name,br_name,
                                           vn_name, vm_name,
                                           &switch_handle,&logical_switch_handle,
                                           &vn_handle,&vm_handle,&switch_ip,&nw_type);
  if(ret_val != RRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"Incomplete Relationships");
    return CRM_ERROR_INCOMPLETE_RESOURCE_RELATIONSHIPS;
  }
  /* notify to reg apps about crm port delete */
  apphookoffset = CRM_NOTIFICATION_APP_HOOK_OFFSET;

  /* fill the notification data */

  strcpy(notification_data.vn_name,vn_name);
  strncpy(notification_data.port_name , crm_port_node_scan_p->port_name, CRM_MAX_PORT_NAME_LEN);
  notification_data.new_port_id       =  crm_port_node_scan_p->port_id;
  notification_data.crm_port_type     =  crm_port_node_scan_p->crm_port_type;
  notification_data.crm_port_status   =  crm_port_node_scan_p->port_status;

  notification_data.crm_port_handle   =  crm_port_node_scan_p->magic;
  notification_data.crm_port_handle   =  ((notification_data.crm_port_handle<<32)|(crm_port_node_scan_p->index));

  notification_data.crm_vn_handle     =  crm_port_node_scan_p->saferef_vn;
  notification_data.dp_handle         =  crm_port_node_scan_p->system_br_saferef;
  notification_data.crm_vm_handle     =  crm_port_node_scan_p->vmInfo.saferef_vm;
  notification_data.nw_type           =  crm_port_node_scan_p->nw_type; 

  OF_LIST_SCAN(crm_port_notifications[CRM_PORT_DELETE], app_entry_p, struct crm_notification_app_hook_info *, apphookoffset )
  {
    ((crm_port_notifier_fn)(app_entry_p->call_back))(CRM_PORT_DELETE,
						     notification_data.crm_port_handle,
    					             notification_data,
    						     app_entry_p->cbk_arg1,
                                                     app_entry_p->cbk_arg2);
  }  
  OF_LIST_SCAN(crm_port_notifications[CRM_VN_ALL_NOTIFICATIONS], app_entry_p, struct crm_notification_app_hook_info *, apphookoffset )
  {
    ((crm_port_notifier_fn)(app_entry_p->call_back))(CRM_PORT_DELETE,
  				          	     notification_data.crm_port_handle,
                                                     notification_data,
                                                     app_entry_p->cbk_arg1,
                                                     app_entry_p->cbk_arg2);
  }

  rrm_delete_vmport_resource_relationships(port_name,port_handle,port_type,switch_name,vm_name,vn_name,vn_handle,logical_switch_handle);

  CNTLR_RCU_READ_LOCK_TAKE();
  
  offset = CRM_PORT_NODE_PORT_TBL_OFFSET;
  MCHASH_DELETE_NODE_BY_REF(crm_port_table_g, crm_port_node_scan_p->index, crm_port_node_scan_p->magic, struct crm_port *, offset,status_b);

  if(status_b == TRUE)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "crm port deleted successfully from the table");
  }
  else
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "crm port failed to delete from the table");
    CNTLR_RCU_READ_LOCK_RELEASE();
    return CRM_FAILURE;
  }

  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_SUCCESS;
}
/******************************************************************************************************/
int32_t crm_get_port_byname(char* port_name_p, struct crm_port ** port_info_p_p)
{
  uint32_t hashkey,offset;
  struct   crm_port* port_info1_p;

  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "entered(portname:%s)",port_name_p);

  CNTLR_RCU_READ_LOCK_TAKE();
  hashkey = crm_get_hashval_byname(port_name_p, crm_no_of_port_buckets_g);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "hashkey:%d\r\n",hashkey);

  offset = CRM_PORT_NODE_PORT_TBL_OFFSET;
  MCHASH_BUCKET_SCAN(crm_port_table_g, hashkey, port_info1_p, struct crm_port *, offset)
  {
    if(!strcmp(port_info1_p->port_name,port_name_p))
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "port name %s found in crm db",port_info1_p->port_name);
      *port_info_p_p = port_info1_p;	
      return CRM_SUCCESS;
    }
    continue;
  }

  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_FAILURE;
}
/*************************************************************************************
* function:crm_get_port_byhandle
* description:
* this function returns virtual network info for given vn_handle.
**************************************************************************************/
int32_t crm_get_port_byhandle(uint64_t port_handle,struct crm_port **crm_port_info_p)
{
  /* caller shall take rcu locks. */
  uint32_t index,magic;
  uint8_t status_b;

  magic = (uint32_t)(port_handle >>32);
  index = (uint32_t)port_handle;

  MCHASH_ACCESS_NODE(crm_port_table_g,index,magic,*crm_port_info_p,status_b);
  
  if(TRUE == status_b)
    return CRM_SUCCESS;
  return CRM_ERROR_INVALID_PORT_HANDLE;
}
/************************************************************************************
* * function:crm_get_port_handle
* * description: 
* * this function returns the virtual network handle for the given vn name
* *
************************************************************************************/
int32_t crm_get_port_handle(char *port_name_p, uint64_t *port_handle_p)
{
  uint32_t hashkey;
  uint32_t offset;

  struct crm_port *crm_port_info_p=NULL;

  if(crm_port_table_g == NULL)
    return CRM_FAILURE;

  hashkey = crm_get_hashval_byname(port_name_p,crm_no_of_port_buckets_g);
  OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "hashkey:%d\r\n",hashkey);
  CNTLR_RCU_READ_LOCK_TAKE();
  offset = CRM_PORT_NODE_PORT_TBL_OFFSET;

  MCHASH_BUCKET_SCAN(crm_port_table_g, hashkey, crm_port_info_p, struct crm_port *,offset)
  {
    if(strcmp(crm_port_info_p->port_name,port_name_p))
      continue;
    CNTLR_RCU_READ_LOCK_RELEASE();

    *port_handle_p = crm_port_info_p->magic;
    *port_handle_p = ((*port_handle_p <<32) | (crm_port_info_p->index));

    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "*port_handle_p:%x\r\n",*port_handle_p);
    return CRM_SUCCESS;
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_ERROR_VN_NAME_INVALID;
}
/************************************************************************************/
void crm_set_port_status(char* crm_port_name, uint64_t crm_port_handle,
                         uint64_t system_port_handle,
                         uint8_t  port_status,
                         uint32_t system_port_id)
{
  int32_t  ret_val;
  uint32_t crm_notification_type;  
  struct   crm_port* crm_port_node_p; 
  struct   crm_port_notification_data  crm_notification_data={};
  uint32_t apphookoffset;
  struct   crm_notification_app_hook_info *app_entry_p=NULL;

  if(port_status == PORT_STATUS_CREATED)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG,"System port with port name(%s) is created and is not ready",crm_port_name);
    return;
  }

  CNTLR_RCU_READ_LOCK_TAKE(); 
  ret_val = crm_get_port_byhandle(crm_port_handle,&crm_port_node_p);
  if(ret_val != CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "port name(%s) not present in the crm database",crm_port_name);
    CNTLR_RCU_READ_LOCK_RELEASE();
    return;
  }

  if((port_status == PORT_STATUS_UP) &&
     (crm_port_node_p->port_id == 0) && 
     (crm_port_node_p->port_status == SYSTEM_PORT_NOT_READY))
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "Datapath is connected to Controller and the crm port is now active");

    /** fill the notification data **/
   
    crm_notification_type         = CRM_PORT_ADDED;
    crm_port_node_p->port_id      = system_port_id;
    crm_port_node_p->port_status  = SYSTEM_PORT_READY;
  
    crm_port_node_p->system_port_handle = system_port_handle;

    /** fill the notification data **/
    crm_notification_data.new_port_id     = crm_port_node_p->port_id;
    crm_notification_data.crm_port_type   = crm_port_node_p->crm_port_type;
    crm_notification_data.crm_port_status = SYSTEM_PORT_READY;
    crm_notification_data.crm_port_handle = crm_port_handle;
    crm_notification_data.crm_vn_handle   = crm_port_node_p->saferef_vn;
    crm_notification_data.dp_handle       = crm_port_node_p->system_br_saferef;
    crm_notification_data.crm_vm_handle   = crm_port_node_p->vmInfo.saferef_vm;
    crm_notification_data.nw_type         = crm_port_node_p->nw_type;

    strncpy(crm_notification_data.port_name ,crm_port_node_p->port_name, CRM_MAX_PORT_NAME_LEN);
    strcpy(crm_notification_data.swname ,crm_port_node_p->switch_name);

    if(crm_notification_data.nw_type == VXLAN_TYPE) 
    {
      crm_notification_data.nw_params.vxlan_nw.service_port = crm_port_node_p->nw_params.vxlan_nw.service_port;
      crm_notification_data.nw_params.vxlan_nw.nid          = crm_port_node_p->nw_params.vxlan_nw.nid;
      crm_notification_data.nw_params.vxlan_nw.remote_ip    = crm_port_node_p->nw_params.vxlan_nw.remote_ip;
    }

    if(crm_notification_data.nw_type == NVGRE_TYPE)
    {
      crm_notification_data.nw_params.nvgre_nw.service_port = crm_port_node_p->nw_params.nvgre_nw.service_port;
      crm_notification_data.nw_params.nvgre_nw.nid          = crm_port_node_p->nw_params.nvgre_nw.nid;
      crm_notification_data.nw_params.nvgre_nw.remote_ip    = crm_port_node_p->nw_params.nvgre_nw.remote_ip;
    }
  }
 
  else if((port_status == PORT_STATUS_DOWN) &&
          (crm_port_node_p->port_id != 0)   &&
          (crm_port_node_p->port_id == system_port_id) &&
          (crm_port_node_p->port_status == SYSTEM_PORT_READY))
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "port present andready in the crm db, but port deleted in system module");

    crm_notification_type                 = CRM_PORT_DELETE;
    crm_notification_data.new_port_id     = crm_port_node_p->port_id;
    crm_notification_data.crm_port_type   = crm_port_node_p->crm_port_type;
    crm_port_node_p->port_id              = 0;
    crm_port_node_p->port_status          = SYSTEM_PORT_NOT_READY;

    /** crm port notification data **/
    crm_notification_data.crm_port_status = SYSTEM_PORT_NOT_READY;
    crm_notification_data.crm_port_handle = crm_port_node_p->magic;
    crm_notification_data.crm_port_handle = ((crm_notification_data.crm_port_handle<<32)|(crm_port_node_p->index));
    crm_notification_data.crm_vn_handle   = crm_port_node_p->saferef_vn;
    crm_notification_data.dp_handle       = crm_port_node_p->system_br_saferef;
    crm_notification_data.crm_vm_handle   = crm_port_node_p->vmInfo.saferef_vm;
    crm_port_node_p->system_port_handle   = 0;
    crm_notification_data.nw_type         = crm_port_node_p->nw_type;
   
    strcpy(crm_notification_data.port_name ,crm_port_node_p->port_name);
    strcpy(crm_notification_data.swname ,crm_port_node_p->switch_name);

    if(crm_notification_data.nw_type == VXLAN_TYPE)
    {
      crm_notification_data.nw_params.vxlan_nw.service_port = crm_port_node_p->nw_params.vxlan_nw.service_port;
      crm_notification_data.nw_params.vxlan_nw.nid          = crm_port_node_p->nw_params.vxlan_nw.nid;
      crm_notification_data.nw_params.vxlan_nw.remote_ip    = crm_port_node_p->nw_params.vxlan_nw.remote_ip;
    }

    if(crm_notification_data.nw_type == NVGRE_TYPE)
    {
      crm_notification_data.nw_params.nvgre_nw.service_port = crm_port_node_p->nw_params.nvgre_nw.service_port;
      crm_notification_data.nw_params.nvgre_nw.nid          = crm_port_node_p->nw_params.nvgre_nw.nid;
      crm_notification_data.nw_params.nvgre_nw.remote_ip    = crm_port_node_p->nw_params.nvgre_nw.remote_ip;
    }
  }

  if(port_status == PORT_STATUS_UPDATE)
  {
    if((crm_port_node_p->port_id == 0) &&
       (crm_port_node_p->port_status == SYSTEM_PORT_NOT_READY) &&
       (system_port_id != 0))
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "port  presents in system and crm . now port ready in the system database.");
      crm_notification_type = CRM_PORT_ADDED;
      crm_port_node_p->port_status = SYSTEM_PORT_READY;
    }
    else if((crm_port_node_p->port_id != 0) &&
        (crm_port_node_p->port_status == SYSTEM_PORT_READY) &&
        (system_port_id != 0)&&
        (crm_port_node_p->port_id != system_port_id)
        )
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "port  presents in crm and system .  already port ready in the system database. but now port id value changed.");
      /** copy previous crm port id to send in the crm notification data */
      crm_notification_data.old_port_id = crm_port_node_p->port_id;
      crm_notification_type = CRM_PORT_MODIFIED;
    }
    /* Datapath reconnected. */
    else if((crm_port_node_p->port_id != 0) &&
        (crm_port_node_p->port_status == SYSTEM_PORT_READY) &&
        (system_port_id != 0) &&
        (crm_port_node_p->port_id == system_port_id)
        )
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "port presentin crm and system.datapath reconnected.port id is same as previous if-else takes care of it.");

      /** copy previous crm port id to send in the crm notification data */
      //crm_notification_data.old_port_id = crm_port_node_p->port_id;
      crm_notification_type = CRM_PORT_ADDED; /* TBD NSM Explore more whilte testing */
    }
    else
    {
      OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "*** Explore more ***");
      CNTLR_RCU_READ_LOCK_RELEASE();
      return;
    }
  
    crm_port_node_p->port_id = system_port_id;
    crm_port_node_p->port_status = SYSTEM_PORT_READY;

    /** fill the notification data **/
    crm_notification_data.new_port_id     =  crm_port_node_p->port_id;
    crm_notification_data.crm_port_type   =  crm_port_node_p->crm_port_type;
    crm_notification_data.crm_port_status =  SYSTEM_PORT_READY;
    crm_notification_data.crm_port_handle =  crm_port_node_p->magic;
    crm_notification_data.crm_port_handle =  ((crm_notification_data.crm_port_handle<<32)|(crm_port_node_p->index));
    crm_notification_data.crm_vn_handle   =  crm_port_node_p->saferef_vn;
    crm_notification_data.dp_handle       =  crm_port_node_p->system_br_saferef;
    crm_notification_data.crm_vm_handle   =  crm_port_node_p->vmInfo.saferef_vm;
    crm_notification_data.nw_type         =  crm_port_node_p->nw_type;
   
    strncpy(crm_notification_data.port_name ,crm_port_node_p->port_name, CRM_MAX_PORT_NAME_LEN);
    strcpy(crm_notification_data.swname ,crm_port_node_p->switch_name);

    if(crm_notification_data.nw_type == VXLAN_TYPE)
    {
      crm_notification_data.nw_params.vxlan_nw.service_port = crm_port_node_p->nw_params.vxlan_nw.service_port;
      crm_notification_data.nw_params.vxlan_nw.nid          = crm_port_node_p->nw_params.vxlan_nw.nid;
      crm_notification_data.nw_params.vxlan_nw.remote_ip    = crm_port_node_p->nw_params.vxlan_nw.remote_ip;
    }

    if(crm_notification_data.nw_type == NVGRE_TYPE)
    {
      crm_notification_data.nw_params.nvgre_nw.service_port = crm_port_node_p->nw_params.nvgre_nw.service_port;
      crm_notification_data.nw_params.nvgre_nw.nid          = crm_port_node_p->nw_params.nvgre_nw.nid;
      crm_notification_data.nw_params.nvgre_nw.remote_ip    = crm_port_node_p->nw_params.nvgre_nw.remote_ip;
    }
  }
  apphookoffset = CRM_NOTIFICATION_APP_HOOK_OFFSET;
  OF_LIST_SCAN(crm_port_notifications[crm_notification_type], app_entry_p, struct crm_notification_app_hook_info *, apphookoffset )
  {
    ((crm_port_notifier_fn)(app_entry_p->call_back))(crm_notification_type,
                                                     crm_notification_data.crm_port_handle,
                                                     crm_notification_data,
                                                     app_entry_p->cbk_arg1,
                                                     app_entry_p->cbk_arg2);
  }

  OF_LIST_SCAN(crm_port_notifications[CRM_VN_ALL_NOTIFICATIONS], app_entry_p, struct crm_notification_app_hook_info *, apphookoffset )
  {
    ((crm_port_notifier_fn)(app_entry_p->call_back))(crm_notification_type,
                                                     crm_notification_data.crm_port_handle,
                                                     crm_notification_data,
                                                     app_entry_p->cbk_arg1,
                                                     app_entry_p->cbk_arg2);
  }
}
/*****************************************************************************************************
* function : crm_register_port_notifications
* description:
*****************************************************************************************************/
int32_t crm_register_port_notifications (uint32_t notification_type,
                                         crm_port_notifier_fn port_notifier_fn,
                                         void* callback_arg1,
                                         void* callback_arg2)
{
  struct   crm_notification_app_hook_info *app_entry_p;
  uint8_t  heap_b;
  int32_t  retval = CRM_SUCCESS;
  uchar8_t lstoffset;
  lstoffset = CRM_NOTIFICATION_APP_HOOK_OFFSET;

  if((notification_type < CRM_PORT_FIRST_NOTIFICATION_TYPE) ||
      (notification_type > CRM_PORT_LAST_NOTIFICATION_TYPE))
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "invalid notification type:%d", notification_type);
    return CRM_FAILURE;
  }

  if(port_notifier_fn == NULL)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "callback function is NULL");
    return CRM_FAILURE;
  }

  retval = mempool_get_mem_block(crm_notifications_mempool_g, (uchar8_t**)&app_entry_p, &heap_b);
  if(retval != MEMPOOL_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "failed to get mempool");
    return CRM_FAILURE;
  }

  app_entry_p->call_back = (void*)port_notifier_fn;
  app_entry_p->cbk_arg1  = callback_arg1;
  app_entry_p->cbk_arg2  = callback_arg2;
  app_entry_p->heap_b    = heap_b;

  /* add application to the  of virtual network notifications */
  OF_APPEND_NODE_TO_LIST(crm_port_notifications[notification_type], app_entry_p, lstoffset);

  if(retval != CRM_SUCCESS)
    mempool_release_mem_block(crm_notifications_mempool_g, (uchar8_t*)app_entry_p, app_entry_p->heap_b);
  return retval;
}
/*************************************************************************************************/
int32_t crm_deregister_port_notifications(uint32_t notification_type,
                                          crm_port_notifier_fn port_notifier_fn)
{
  struct crm_notification_app_hook_info *app_entry_p,*prev_app_entry_p = NULL;
  uchar8_t lstoffset;
  lstoffset = CRM_NOTIFICATION_APP_HOOK_OFFSET;

  if((notification_type < CRM_PORT_FIRST_NOTIFICATION_TYPE) ||
     (notification_type > CRM_PORT_LAST_NOTIFICATION_TYPE))
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "invalid port notification type:%d", notification_type);
    return CRM_FAILURE;
  }

  if(port_notifier_fn == NULL)
  {
    OF_LOG_MSG(OF_LOG_CRM,OF_LOG_DEBUG, "callback function is NULL");
    return CRM_FAILURE;
  }

  CNTLR_RCU_READ_LOCK_TAKE();
  OF_LIST_SCAN(crm_port_notifications[notification_type],app_entry_p,struct crm_notification_app_hook_info *,lstoffset )
  {
    if(app_entry_p->call_back != port_notifier_fn)
    {
      prev_app_entry_p = app_entry_p;
      continue;
    }
    OF_REMOVE_NODE_FROM_LIST(crm_port_notifications[notification_type],app_entry_p,prev_app_entry_p,lstoffset );
    CNTLR_RCU_READ_LOCK_RELEASE();
    return CRM_SUCCESS;
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
  return CRM_FAILURE;
}
/***************************************************************************************************************/
#if 0
TBD NSM get first vmside port and next vmside port from

1. vn->computenode->logicalswitch->vm_ports
2. vn->computenode->logicalswitch->vmns_ports

API is to be defined here. But Actual implementation is in resource_relationships_mgr.c
as it needs to fetch using vn API.
#endif
/***************************************************************************************************************/
int32_t crm_add_attribute_to_port(char* port_name_p, struct crm_attribute_name_value_pair* attribute_info_p)
{
  struct    crm_port                        *port_entry_p;
  struct    crm_port_notification_data       notification_data;
  struct    crm_notification_app_hook_info  *app_entry_p;
  int32_t   retval;
  uint64_t  port_handle;
  uint32_t  lstoffset;

  lstoffset = CRM_NOTIFICATION_APP_HOOK_OFFSET;

  CNTLR_RCU_READ_LOCK_TAKE();
  do
  {
    retval = crm_get_port_handle(port_name_p ,&port_handle);
    if(retval != CRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
    retval = crm_get_port_byhandle(port_handle, &port_entry_p);
    if(retval != CRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
    retval = crm_add_attribute(&(port_entry_p->attributes),attribute_info_p);
    if(retval != CRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
    port_entry_p->number_of_attributes++;
//  of_create_crm_view_entry(port_handle, attribute_info_p->name_string, attribute_info_p->value_string);
    strcpy(notification_data.port_name, port_entry_p->port_name);
    strcpy(notification_data.attribute_name,attribute_info_p->name_string);
    strcpy(notification_data.attribute_value,attribute_info_p->value_string);
    OF_LIST_SCAN(crm_port_notifications[CRM_PORT_ATTRIBUTE_ADDED],app_entry_p,struct crm_notification_app_hook_info*,lstoffset )
    {
      ((crm_port_notifier_fn)(app_entry_p->call_back))(CRM_PORT_ATTRIBUTE_ADDED,
                                                       port_handle,
                                                       notification_data,
                                                       app_entry_p->cbk_arg1,
                                                       app_entry_p->cbk_arg2);
    }
     OF_LIST_SCAN(crm_port_notifications[CRM_PORT_ALL_NOTIFICATIONS],app_entry_p,struct crm_notification_app_hook_info*,lstoffset )
    {
      ((crm_port_notifier_fn)(app_entry_p->call_back))(CRM_PORT_ATTRIBUTE_ADDED,
                                                       port_handle,
                                                       notification_data,
                                                       app_entry_p->cbk_arg1,
                                                       app_entry_p->cbk_arg2);
    }
  }while(0);

  if(retval != CRM_SUCCESS)
  {
    OF_LOG_MSG(OF_LOG_CRM, OF_LOG_DEBUG,"Attribute addition failed");
    CNTLR_RCU_READ_LOCK_RELEASE();
    return retval;
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
  OF_LOG_MSG(OF_LOG_CRM, OF_LOG_DEBUG,"Attribute addition succesful");
  return retval;
}

/******************************************************************************************************/
int32_t crm_delete_attribute_from_port(char* port_name_p, struct crm_attribute_name_value_pair *attribute_info_p)
{

  struct   crm_port                        *port_entry_p;
  struct   crm_resource_attribute_entry    *attribute_entry_p, *prev_app_entry_p;
  struct   crm_port_notification_data       notification_data;
  struct   crm_notification_app_hook_info  *app_entry_p;

  int32_t  retval;
  uint64_t port_handle;
  uint32_t lstoffset;

  lstoffset = CRM_RESOURCE_ATTRIBUTE_ENTRY_LISTNODE_OFFSET;

  CNTLR_RCU_READ_LOCK_TAKE();
  retval = crm_get_port_handle(port_name_p ,&port_handle);
  if(retval != CRM_SUCCESS)
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    return retval;
  }
  retval = crm_get_port_byhandle(port_handle, &port_entry_p);
  if(retval != CRM_SUCCESS)
  {
    CNTLR_RCU_READ_LOCK_RELEASE();
    return retval;
  }

  OF_LIST_SCAN(port_entry_p->attributes, attribute_entry_p, struct crm_resource_attribute_entry*,lstoffset)
  {
    if(!strcmp(attribute_entry_p->name, attribute_info_p->name_string))
    {
      strcpy(notification_data.port_name, port_entry_p->port_name);
      strcpy(notification_data.attribute_name,attribute_entry_p->name);
      strcpy(notification_data.attribute_value,attribute_entry_p->value);
      OF_LIST_SCAN(crm_port_notifications[CRM_PORT_ATTRIBUTE_DELETE], app_entry_p,
                   struct crm_notification_app_hook_info*,lstoffset )
      {
        ((crm_port_notifier_fn)(app_entry_p->call_back))(CRM_PORT_ATTRIBUTE_DELETE,
                                                         port_handle,
                                                         notification_data,
                                                         app_entry_p->cbk_arg1,
                                                         app_entry_p->cbk_arg2);
      }
      OF_LIST_SCAN(crm_port_notifications[CRM_PORT_ALL_NOTIFICATIONS],app_entry_p,
                   struct crm_notification_app_hook_info*,lstoffset)
      {
        ((crm_port_notifier_fn)(app_entry_p->call_back))(CRM_PORT_ATTRIBUTE_DELETE,
                                                         port_handle,
                                                         notification_data,
                                                         app_entry_p->cbk_arg1,
                                                         app_entry_p->cbk_arg2);
      }
      OF_REMOVE_NODE_FROM_LIST(port_entry_p->attributes,attribute_entry_p,prev_app_entry_p,lstoffset);
      port_entry_p->number_of_attributes--;

      //of_remove_crm_view_entry(attribute_entry_p->name, attribute_entry_p->value);
      CNTLR_RCU_READ_LOCK_RELEASE();
      OF_LOG_MSG(OF_LOG_CRM, OF_LOG_DEBUG,"Attribute deletion successful");
      return CRM_SUCCESS;
    }
    prev_app_entry_p = attribute_entry_p;
    continue;
  }
  CNTLR_RCU_READ_LOCK_RELEASE();
  OF_LOG_MSG(OF_LOG_CRM, OF_LOG_DEBUG,"Attribute deletion failed");
  return CRM_FAILURE;
}

/******************************************************************************************************/
int32_t crm_get_port_first_attribute(char* port_name_p,
                                     struct   crm_attribute_name_value_output_info *attribute_output_p)
{
  struct   crm_port     *port_entry_p;
  int32_t  retval;
  uint64_t port_handle;

  if(attribute_output_p == NULL)
    return CRM_FAILURE;

  CNTLR_RCU_READ_LOCK_TAKE();
  do
  {
    retval = crm_get_port_handle(port_name_p ,&port_handle);
    if(retval != CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_CRM, OF_LOG_DEBUG,"crm_get_port_handle failed");
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
    retval = crm_get_port_byhandle(port_handle, &port_entry_p);
    if(retval != CRM_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_CRM, OF_LOG_DEBUG,"crm_get_port_byhandle failed");
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
    retval = crm_get_first_attribute(&(port_entry_p->attributes), attribute_output_p);
  }while(0);
  CNTLR_RCU_READ_LOCK_RELEASE();
  return retval;
}

/******************************************************************************************************/
int32_t crm_get_port_next_attribute(char*    port_name_p,
                                    char*    current_attribute_name,
                                    struct   crm_attribute_name_value_output_info *attribute_output_p)
{
  struct   crm_port     *port_entry_p;
  int32_t  retval;
  uint64_t port_handle;

  if((attribute_output_p == NULL) || (current_attribute_name == NULL))
    return CRM_FAILURE;

  CNTLR_RCU_READ_LOCK_TAKE();
  do
  {
    retval = crm_get_port_handle(port_name_p ,&port_handle);
    if(retval != CRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
    retval = crm_get_port_byhandle(port_handle, &port_entry_p);
    if(retval != CRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
    retval = crm_get_next_attribute(&(port_entry_p->attributes), current_attribute_name,
                                      attribute_output_p);
  }while(0);
  CNTLR_RCU_READ_LOCK_RELEASE();
  return retval;
}
/******************************************************************************************************/
int32_t crm_get_port_exact_attribute(char* port_name_p,
                                     char* attribute_name_string)
{
  struct   crm_port     *port_entry_p;
  int32_t  retval;
  uint64_t port_handle;

  if(attribute_name_string == NULL)
    return CRM_ERROR_ATTRIBUTE_NAME_NULL;
  if(strlen(attribute_name_string) > CRM_MAX_ATTRIBUTE_NAME_LEN)
    return CRM_ERROR_INVALID_ATTRIBUTE_NAME_LEN;

  CNTLR_RCU_READ_LOCK_TAKE();
  do
  {
    retval = crm_get_port_handle(port_name_p ,&port_handle);
    if(retval != CRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
    retval = crm_get_port_byhandle(port_handle, &port_entry_p);
    if(retval != CRM_SUCCESS)
    {
      CNTLR_RCU_READ_LOCK_RELEASE();
      break;
    }
    retval = crm_get_exact_attribute(&(port_entry_p->attributes),  attribute_name_string);
  }while(0);
  CNTLR_RCU_READ_LOCK_RELEASE();
  return retval;
}
/****************************************************************************************************/

