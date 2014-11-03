/**
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

/*!\file of_cntlr_2_sw_msgapi.c
* Author     : Rajesh Madabushi <rajesh.madabushi@freescale.com>
* Date       : November, 2012  
* Description: 
* This file contains defintions different controller to switch messaging APIs provided to 
* North bound interface. 
*/

#include "controller.h"
#include "of_utilities.h"
#include "of_msgapi.h"
#include "of_multipart.h"
#include "cntrl_queue.h"
#include "of_msgpool.h"
#include "cntlr_tls.h"
#include "cntlr_transprt.h"
#include "cntlr_event.h"
#include "dprmldef.h"
#include "multi_part.h"
#include "oflog.h"
#include "of_group.h"
#include "fsl_ext.h"
#include "nicira_ext.h"
#define OF_OXM_HDR_LEN   (4)

void of_group_free_bucket_entry(struct rcu_head *node);

void of_group_free_action_entry(struct rcu_head *node);

void of_group_free_bucket_prop_entry(struct rcu_head *node);


/* add_group with/without bucket, bucket properties
   and actions.  */

int32_t
of_frame_and_send_add_group_info(uint64_t datapath_handle,
                                struct ofi_group_desc_info *group_info,
                                struct ofi_bucket *bucket_info,
                                struct ofi_bucket_prop *bucket_prop,
                                struct ofi_action *action_info)
{
    struct of_msg *msg = NULL;
    void *conn_info_p;
    uint16_t  msg_len; 
    uint16_t length_of_buckets=0;
    int32_t status = OF_SUCCESS;
    int32_t retval = OF_SUCCESS;
  
         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s",__FUNCTION__);
    msg_len = OFU_ADD_OR_MODIFY_OR_DELETE_GROUP_MESSAGE +
              OFU_GROUP_ACTION_BUCKET_LEN +
              OFU_OUTPUT_ACTION_LEN +
              OFU_GROUP_BUCKET_PROP_WEIGHT_LEN;
    do
    {
         msg = ofu_alloc_message(OFPT_GROUP_MOD, msg_len);
         if (msg == NULL)
         {
            OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"OF message alloc error");
            status= OF_FAILURE;
            break;
         }

         if ((bucket_info != NULL) || (action_info != NULL))
         {
            retval = of_frame_actions_and_buckets(msg,group_info, bucket_info,
                                       bucket_prop, action_info, &length_of_buckets);
            if ( retval != OF_SUCCESS)
            {
               OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR," Creation of bucket for multicast traffic failed");
               status=retval;
               break;
            }
         }

        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s %d length_of_buckets=%d",__FUNCTION__,__LINE__,length_of_buckets);
         /*Create group mod message with ADD command with the action buckets created above*/
         retval=of_add_group(msg, datapath_handle,group_info->group_id, group_info->group_type,
                     length_of_buckets,&conn_info_p);
         if ( retval != OFU_ADD_GROUP_ENTRY_SUCCESS)
         {
             OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"add_group failed");
             status=retval;
             break;
         }

         /* Sending the group  mod message with add command to dp*/
         retval=of_cntrlr_send_request_message(msg, datapath_handle, conn_info_p,NULL, NULL);
         if ( retval != OF_SUCCESS)
         {
             OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR," Sending Add group command failed");
             status=retval;
             break;
         }
    }
    while(0);

    if(status != OF_SUCCESS)
    {
         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s ERROR",__FUNCTION__);
      if (msg)
           msg->desc.free_cbk(msg);
    }

         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s %d",__FUNCTION__,__LINE__);
    return status;
}


/* mod_group with/without bucket, bucket properties
   and actions.  */

int32_t
of_frame_and_send_mod_group_bucket_info(uint64_t datapath_handle,
                                struct ofi_group_desc_info *group_info,
                                struct ofi_bucket *bucket_info,
                                struct ofi_bucket_prop *bucket_prop,
                                struct ofi_action *action_info, 
                                uint32_t  command_bkt_id)
{
    struct of_msg *msg = NULL;
    void *conn_info_p;
    uint16_t  msg_len; 
    uint16_t length_of_buckets=0;
    int32_t status = OF_SUCCESS;
    int32_t retval = OF_SUCCESS;

         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s",__FUNCTION__);
    /* Creating group with one bucket with actions OUTPUT to Port */
    msg_len = OFU_ADD_OR_MODIFY_OR_DELETE_GROUP_MESSAGE +
              OFU_GROUP_ACTION_BUCKET_LEN +
              OFU_OUTPUT_ACTION_LEN +
              OFU_GROUP_BUCKET_PROP_WEIGHT_LEN;
    do
    {
         msg = ofu_alloc_message(OFPT_GROUP_MOD, msg_len);
         if (msg == NULL)
         {
            OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"OF message alloc error");
            status= OF_FAILURE;
            break;
         }

         if ((group_info != NULL) || (bucket_info != NULL))
         {
           retval = of_frame_actions_and_buckets(msg,group_info, bucket_info, bucket_prop, 
                                               action_info, &length_of_buckets);
           if ( retval != OF_SUCCESS)
           {
               OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR," APpending  of bucket to multicast group failed");
               status=retval;
               break;
           }
         }

         /*Append above created bucket to the group id created earlier for multicast group*/
         retval = of_insert_bucket_to_group(msg, datapath_handle, group_info->group_id,
                      group_info->group_type, command_bkt_id,length_of_buckets,&conn_info_p);
         if ( retval != OFU_INSERT_BUCKET_TO_GROUP_SUCCESS)
         {
             OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"of_insert_bucket_to_group failed");
             status=retval;
             break;
         }

         /* Sending the group  mod message with add command to dp*/
         retval=of_cntrlr_send_request_message(msg, datapath_handle, conn_info_p,NULL, NULL);
         if ( retval != OF_SUCCESS)
         {
             OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR," Sending Add group command failed");
             status=retval;
             break;
         }
    }
    while(0);

    if(status != OF_SUCCESS)
    {
         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s ERROR",__FUNCTION__);
      if (msg)
           msg->desc.free_cbk(msg);
    }

         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s %d",__FUNCTION__,__LINE__);
    return status;
}


int32_t
of_frame_actions_and_buckets(struct of_msg *msg,
                        struct ofi_group_desc_info *group_entry_p,
                        struct ofi_bucket *bucket_entry_p,
                        struct ofi_bucket_prop *bucket_prop_entry_p,
                        struct ofi_action *action_p, 
                        uint16_t *bucket_len)
{
     uint8_t starting_location;
     uint16_t length_of_actions;
     uint16_t length_of_prop;
     uint16_t length_of_buckets;
     int32_t status = OF_SUCCESS;

     OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s %d",__FUNCTION__,__LINE__);

   ofu_start_appending_buckets_to_group(msg);
   if (bucket_entry_p != NULL)
   {
    /* start framing bucket id */
      switch(group_entry_p->group_type)
      {
        case  OFPGT_ALL: /* All (multicast/broadcast) group.  */
          /*Appending bucket of type ALL */
          status=ofu_append_bucket_to_group_type_all(msg, bucket_entry_p->bucket_id);
          break;

        case OFPGT_SELECT: /* Select group. */
	  if(bucket_prop_entry_p->weight == 0){
		  status = OF_GROUP_WEIGHT_PARAM_REQUIRED;
		  break;
 	  }
          status=ofu_append_bucket_to_group_type_select(msg,
              bucket_prop_entry_p->weight);
	  break;

        case OFPGT_INDIRECT: /* Indirect group. */
          status=ofu_append_bucket_to_group_type_indirect(msg, bucket_entry_p->bucket_id);
         // OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"not supported");
          break;

        case OFPGT_FF: /* fast failover group */ 
	  if((bucket_prop_entry_p->watch_port == 0) && (bucket_prop_entry_p->watch_group == 0))
	  {
		  status = OF_GROUP_WATCH_PORT_OR_GROUP_REQUIRED;
		  break;
	  }
          status=ofu_append_bucket_to_group_type_fast_failover(msg, bucket_entry_p->bucket_id);
          break; 

      }

     if (status != OFU_APPEND_BUCKET_SUCCESS)
     {
         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"preparing bucket info failed");
         return OF_FAILURE;
     }
   } /* end framing bucket id */
   
    /* start preparing action message using action type */
    ofu_start_pushing_actions(msg);
    if (action_p != NULL)
    {
    switch(action_p->type)
    {
      case 	OFPAT_OUTPUT:  /* Output to switch port. */
     /*Adding OUTPUT port action to bucket*/
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_OUTPUT");
        status=ofu_push_output_action(msg,action_p->port_no, action_p->max_len);
        if (status != OFU_ACTION_PUSH_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"ofu_push_output_action failed");
          status=OF_FAILURE;
          break;
        }
        break;
    case   OFPAT_COPY_TTL_OUT: /* Copy TTL "outwards" -- from next-to-outermost    to outermost */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_COPY_TTL_OUT");
      status=
        ofu_push_copy_ttl_outward_action(msg);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_copy_ttl_outward_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_COPY_TTL_IN  : /* Copy TTL "inwards" -- from outermost to    next-to-outermost */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_COPY_TTL_IN:");
      status=ofu_push_copy_ttl_inward_action(msg);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"fu_push_inward_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_SET_MPLS_TTL : /* MPLS TTL */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_SET_MPLS_TTL");
      status=
        ofu_push_set_mpls_ttl_action(msg, action_p->ttl);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_WARN,"ofu_push_set_mpls_ttl_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_DEC_MPLS_TTL : /* Decrement MPLS TTL */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_DEC_MPLS_TTL");
      status=
        ofu_push_dec_mpls_ttl_action(msg);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_dec_mpls_ttl_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_PUSH_VLAN  : /* Push a new VLAN tag */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_PUSH_VLAN");

      status=ofu_push_push_vlan_header_action(msg,action_p->ether_type);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_push_vlan_header_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_POP_VLAN : /* Pop the outer VLAN tag */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_POP_MPLS:");

      status=ofu_push_pop_vlan_header_action(msg);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_pop_mpls_header_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_PUSH_MPLS: /* Push a new MPLS tag */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_PUSH_MPLS:");

      status=
        ofu_push_push_mpls_header_action(msg, action_p->ether_type);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_push_mpls_header_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_POP_MPLS : /* Pop the outer MPLS tag */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_POP_MPLS:");
      status=
        ofu_push_pop_mpls_header_action(msg, action_p->ether_type);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_pop_mpls_header_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_SET_QUEUE: /* Set queue id when outputting to a port */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_SET_QUE:");
      status=ofu_push_set_queue_action(msg,action_p->queue_id);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"fu_push_queue_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_GROUP : /* Apply group. */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_GROUP:");
      status=ofu_push_group_action(msg, action_p->group_id);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"fu_push_group_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_SET_NW_TTL : /* IP TTL. */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG," OFPAT_SET_NW_TTL:");
      status=
        ofu_push_set_ipv4_ttl_action(msg,action_p->ttl);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_set_ipv4_ttl_action failed");
        status=OF_FAILURE;
        break;
      }

      break;
    case   OFPAT_DEC_NW_TTL: /* Decrement IP TTL. */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG," OFPAT_DEC_NW_TTL:");
      status=ofu_push_dec_ipv4_ttl_action(msg);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_dec_ipv4_ttl_action failed");
        status=OF_FAILURE;
        break;
      }
      break;

    case   OFPAT_SET_FIELD: /* Set a header field using OXM TLV format. */
      //Added support for setfield
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG," OFPAT_SET_FIELD:");
      status = of_flow_set_action_setfield(msg,action_p);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
	      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR," of_flow_set_action_setfield failed!");
	      status=OF_FAILURE;
	      break;
      }
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"%s(%d) msg->desc.length3:%d",__FUNCTION__,__LINE__,msg->desc.length3);
      break;

    case   OFPAT_PUSH_PBB: /* Push a new PBB service tag (I-TAG) */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_PUSH_PBB:");
      status=
        ofu_push_push_pbb_header_action(msg,action_p->ether_type);

      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_push_pbb_header_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_POP_PBB: /* Pop the outer PBB service tag (I-TAG) */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_POP_PBB:");
      status=
        ofu_push_pop_pbb_header_action(msg);

      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_pop_pbb_header_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case OFPAT_EXPERIMENTER:
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"experiment action message");
        switch(action_p->sub_type)
        {
#ifdef FSLX_COMMON_AND_L3_EXT_SUPPORT
          case FSLXAT_WRITE_META_DATA:
            {
              status=fslx_action_write_metadata(msg,action_p->ui64_data);

              if (status != OFU_ACTION_PUSH_SUCCESS)
              {
                OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"fslx_action_write_metadata failed");
                status=OF_FAILURE;
                break;
              }
            }
            break;
          case FSLXAT_IP_FRAGMENT:
            {
              status=fslx_action_ipfragment(msg,action_p->ui32_data);
              if (status != OFU_ACTION_PUSH_SUCCESS)
              {
                OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"fslx_action_ipfragment failed");
                status=OF_FAILURE;
                break;
              }
            }
	    break;
	  case FSLXAT_SEND_ICMPV4_TIME_EXCEED:
	    {
		    status = fslx_action_send_icmpv4_time_exceed_err_msg(msg, action_p->ui8_data);
		    if (status != OFU_ACTION_PUSH_SUCCESS)
		    {
			    OF_LOG_MSG(OF_LOG_FWD_NF, OF_LOG_ERROR,"Error in sending icmp time exceed action  err = %d",status);
			    status = OF_FAILURE;
			    break;
		    }

	    }
	    break;
	  case NXAST_REG_LOAD:
	    {
		    status = nicira_action_write_register(msg, action_p->ui32_data, (uint64_t) action_p->port_no, action_p->ui8_data,(uint8_t)action_p->ui16_data );
		    if(status != OFU_SET_FIELD_SUCCESS)
		    {
			    OF_LOG_MSG(OF_LOG_FWD_NF, OF_LOG_ERROR,"action nicira register 0 write data failed,err = %d",status);
			    status = OF_FAILURE;
			    break;
		    }

	    }
	    break;
	  case FSLXAT_DROP_PACKET:
	    {
		    status = fslx_action_drop_packet(msg);
		    if(status != OFU_ACTION_PUSH_SUCCESS)
		    {
			    OF_LOG_MSG(OF_LOG_FWD_NF, OF_LOG_ERROR,"action drop packet failed,err = %d",status);
			    status = OF_FAILURE;
			    break;
		    }
	    }
	    break;
#endif
	 default:
	    {
		    OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"unknown freescale extention");
		    status=OF_FAILURE;
		    break;
	    }
	}
      }
      break;
    }

     if (status != OFU_ACTION_PUSH_SUCCESS)
     {
         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_output_action failed");
         return OF_FAILURE;
     }

    }
     ofu_end_pushing_actions(msg, &starting_location, &length_of_actions);
     ((struct ofp_bucket*)(msg->desc.ptr2))->len = htons(length_of_actions);
     OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"length_of_actions=%d",length_of_actions);
    
  if (bucket_prop_entry_p != NULL)
  {
     /* Start pushing bucket properties */
      ofu_start_attach_bucket_properties(msg);

      if (bucket_prop_entry_p->weight > 0 )
      {
          status = ofu_attach_weight_prop_to_bucket(msg, bucket_prop_entry_p->weight);
      }
      else if (bucket_prop_entry_p->watch_port > 0)
      {
          status = ofu_attach_watch_port_prop_to_bucket(msg, 
                                bucket_prop_entry_p->watch_port);
      }
      else if (bucket_prop_entry_p->watch_group > 0 )
      {
          status = ofu_attach_watch_group_prop_to_bucket(msg, 
                           bucket_prop_entry_p->watch_group);
      }

     if (status != OFU_APPEND_BUCKET_SUCCESS)
     {
         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_attach_weight_prop_to_bucket failed");
         return OF_FAILURE;
     }
     /* End pushing bucket properties */
     ofu_end_attach_bucket_properties(msg, &starting_location, &length_of_prop);
   }
   
     /* End Appending buckets */
     ofu_end_appending_buckets_to_group(msg,&starting_location,&length_of_buckets);

     *bucket_len = length_of_buckets;

  OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s %d length_of_buckets=%d *bucket_len=%d",__FUNCTION__,__LINE__,length_of_buckets,*bucket_len);
         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s %d",__FUNCTION__,__LINE__);

     return OF_SUCCESS;
}

/*Delete Bucket */
int32_t
of_frame_and_send_del_bucket_info(uint64_t datapath_handle,
                                struct ofi_group_desc_info *group_info,
                                struct ofi_bucket *bucket_info,
                                uint32_t  command_bkt_id)
{
    struct of_msg *msg = NULL;
    void *conn_info_p;
    uint16_t  msg_len; 
    int32_t status = OF_SUCCESS;
    int32_t retval = OF_SUCCESS;

         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ENTRY");
    /* Creating group with delete bucket command */
    msg_len = OFU_ADD_OR_MODIFY_OR_DELETE_GROUP_MESSAGE;

    do
    {
         msg = ofu_alloc_message(OFPT_GROUP_MOD, msg_len);
         if (msg == NULL)
         {
            OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"OF message alloc error");
            status= OF_FAILURE;
            break;
         }

        /*Remove  earlier created bucket for the given port of multicast group  from the group id */
         retval = of_remove_bucket_from_group(msg, datapath_handle, group_info->group_id,
                                 group_info->group_type, command_bkt_id, &conn_info_p);
         if ( retval != OFU_REMOVE_BUCKET_FROM_GROUP_SUCCESS)
         {
             OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"add_group failed");
             status=retval;
             break;
         }

         /* Sending the group  mod message with remove bucket command to dp*/
         retval=of_cntrlr_send_request_message(msg, datapath_handle, conn_info_p,NULL, NULL);
         if ( retval != OF_SUCCESS)
         {
             OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,
                        "Sending Remove bucket(%d) command from group(=%x)failed",bucket_info->bucket_id, group_info->group_id);
             status=retval;
             break;
         }
    }
    while(0);

    if(status != OF_SUCCESS)
    {
         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s ERROR",__FUNCTION__);
      if (msg)
           msg->desc.free_cbk(msg);
    }

    return status;
}

/*Delete group */
int32_t
of_frame_and_send_delete_group(uint64_t datapath_handle,
                                 uint32_t group_id)
{
    struct of_msg *msg = NULL;
    void *conn_info_p;
    uint16_t  msg_len; 
    int32_t status = OF_SUCCESS;
    int32_t retval = OF_SUCCESS;

         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"Entry");
    /* Creating group with delete bucket command */
    msg_len = OFU_ADD_OR_MODIFY_OR_DELETE_GROUP_MESSAGE;

    do
    {
         msg = ofu_alloc_message(OFPT_GROUP_MOD, msg_len);
         if (msg == NULL)
         {
            OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"OF message alloc error");
            status= OF_FAILURE;
            break;
         }

         /*Remove  earlier created  multicast group id */
         retval=of_delete_group(msg,datapath_handle, group_id, &conn_info_p);
         if (retval != OFU_DELETE_GROUP_ENTRY_SUCCESS)
         {
             OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,
                        "IGMP:deletion of multicast group(=%x) failed",group_id);
             status=retval;
             break;
         }

         /* Sending the group  mod message with deletion of group command to dp*/
         retval=of_cntrlr_send_request_message(msg, datapath_handle, conn_info_p,NULL, NULL);
         if ( retval != OF_SUCCESS)
         {
             OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR," Sending Deletion of group command failed");
             status=retval;
             break;
         }
    }
    while(0);

    if(status != OF_SUCCESS)
    {
         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ERROR");
      if (msg)
           msg->desc.free_cbk(msg);
    }

         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"Exit...");
    return status;
}

#if 0
int32_t
of_frame_and_send_mod_group_action_info(uint64_t datapath_handle,
                                struct ofi_group_desc_info *group_info,
                                struct ofi_bucket *bucket_info,
                                struct ofi_action *action_info, 
                                uint32_t  command_bkt_id)
{
    struct of_msg *msg = NULL;
    void *conn_info_p;
    uint16_t  msg_len; 
    uint16_t length_of_buckets=0;
    int32_t status = OF_SUCCESS;
    int32_t retval = OF_SUCCESS;

         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s",__FUNCTION__);
    /* Creating group with one action using actions type */
    msg_len = OFU_ADD_OR_MODIFY_OR_DELETE_GROUP_MESSAGE +
              OFU_GROUP_ACTION_BUCKET_LEN +
              OFU_OUTPUT_ACTION_LEN;//+ OFU_GROUP_BUCKET_PROP_WEIGHT_LEN;
    do
    {
         msg = ofu_alloc_message(OFPT_GROUP_MOD, msg_len);
         if (msg == NULL)
         {
            OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"OF message alloc error");
            status= OF_FAILURE;
            break;
         }

         if ((group_info != NULL) || (bucket_info != NULL) || (action_info != NULL))
         {
           retval = of_frame_actions_info(msg, action_info, &length_of_buckets);
           if ( retval != OF_SUCCESS)
           {
               OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR," APpending  of bucket to multicast group failed");
               status=retval;
               break;
           }
         }

         /*Append above created action to the group id created earlier for multicast group*/
         retval=of_insert_bucket_to_group(msg, datapath_handle, group_info->group_id,
                                 /*TODO replace command_bkt_id with OFPG_BUCKET_LAST */
                             group_info->group_type, OFPG_BUCKET_LAST,length_of_buckets,&conn_info_p);
         if ( retval != OFU_INSERT_BUCKET_TO_GROUP_SUCCESS)
         {
             OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"of_insert_bucket_to_group failed");
             status=retval;
             break;
         }

         retval =  of_modify_group(msg, datapath_handle, group_info->group_id,
                           group_info->group_type, length_of_buckets, &conn_info_p);
         if ( retval != OFU_MODIFY_GROUP_ENTRY_SUCCESS)
         {
             OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"of_insert_bucket_to_group failed");
             status=retval;
             break;
         }

         /* Sending the group  mod message with add command to dp*/
         retval=of_cntrlr_send_request_message(msg, datapath_handle, conn_info_p,NULL, NULL);
         if ( retval != OF_SUCCESS)
         {
             OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR," Sending Add group command failed");
             status=retval;
             break;
         }
    }
    while(0);

    if(status != OF_SUCCESS)
    {
         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s ERROR",__FUNCTION__);
      if (msg)
           msg->desc.free_cbk(msg);
    }

         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s %d",__FUNCTION__,__LINE__);
    return status;
}

int32_t
of_frame_actions_info(struct of_msg *msg,
                      struct ofi_action *action_p, 
                      uint16_t *bucket_len)
{
     uint8_t starting_location;
     uint16_t length_of_actions;
     uint16_t length_of_buckets = 0;
     int32_t status = OF_SUCCESS;

         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s %d",__FUNCTION__,__LINE__);

    if (action_p != NULL)
    {
     /*start pushing actions */
     ofu_start_pushing_actions(msg);
    /* start preparing action message using action type */
    switch(action_p->type)
    {
      case 	OFPAT_OUTPUT:  /* Output to switch port. */
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_OUTPUT");
        status=ofu_push_output_action(msg,action_p->port_no, action_p->max_len);
        if (status != OFU_ACTION_PUSH_SUCCESS)
        {
          OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"ofu_push_output_action failed");
          status=OF_FAILURE;
          break;
        }
        break;
    case   OFPAT_COPY_TTL_OUT: /* Copy TTL "outwards" -- from next-to-outermost    to outermost */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_COPY_TTL_OUT");
      status=
        ofu_push_copy_ttl_outward_action(msg);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_copy_ttl_outward_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_COPY_TTL_IN  : /* Copy TTL "inwards" -- from outermost to    next-to-outermost */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_COPY_TTL_IN:");
      status=ofu_push_copy_ttl_inward_action(msg);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"fu_push_inward_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_SET_MPLS_TTL : /* MPLS TTL */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_SET_MPLS_TTL");
      status=
        ofu_push_set_mpls_ttl_action(msg, action_p->ttl);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_WARN,"ofu_push_set_mpls_ttl_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_DEC_MPLS_TTL : /* Decrement MPLS TTL */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_DEC_MPLS_TTL");
      status=
        ofu_push_dec_mpls_ttl_action(msg);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_dec_mpls_ttl_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_PUSH_VLAN  : /* Push a new VLAN tag */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_PUSH_VLAN");

      status=ofu_push_push_vlan_header_action(msg,action_p->ether_type);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_push_vlan_header_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_POP_VLAN : /* Pop the outer VLAN tag */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_POP_MPLS:");

      status=ofu_push_pop_vlan_header_action(msg);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_pop_mpls_header_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_PUSH_MPLS: /* Push a new MPLS tag */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_PUSH_MPLS:");

      status=
        ofu_push_push_mpls_header_action(msg, action_p->ether_type);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_push_mpls_header_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_POP_MPLS : /* Pop the outer MPLS tag */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_POP_MPLS:");
      status=
        ofu_push_pop_mpls_header_action(msg, action_p->ether_type);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_pop_mpls_header_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_SET_QUEUE: /* Set queue id when outputting to a port */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_SET_QUE:");
      status=ofu_push_set_queue_action(msg,action_p->queue_id);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"fu_push_queue_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_GROUP : /* Apply group. */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_GROUP:");
      status=ofu_push_group_action(msg, action_p->group_id);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"fu_push_group_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_SET_NW_TTL : /* IP TTL. */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG," OFPAT_SET_NW_TTL:");
      status=
        ofu_push_set_ipv4_ttl_action(msg,action_p->ttl);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_set_ipv4_ttl_action failed");
        status=OF_FAILURE;
        break;
      }

      break;
    case   OFPAT_DEC_NW_TTL: /* Decrement IP TTL. */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG," OFPAT_DEC_NW_TTL:");
      status=ofu_push_dec_ipv4_ttl_action(msg);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_dec_ipv4_ttl_action failed");
        status=OF_FAILURE;
        break;
      }
      break;

    case   OFPAT_SET_FIELD: /* Set a header field using OXM TLV format. */
      //Added support for setfield
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG," OFPAT_SET_FIELD:");
      status = of_flow_set_action_setfield(msg,action_p);
      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
	      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR," of_flow_set_action_setfield failed!");
	      status=OF_FAILURE;
	      break;
      }
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"%s(%d) msg->desc.length3:%d",__FUNCTION__,__LINE__,msg->desc.length3);
      break;

    case   OFPAT_PUSH_PBB: /* Push a new PBB service tag (I-TAG) */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_PUSH_PBB:");
      status=
        ofu_push_push_pbb_header_action(msg,action_p->ether_type);

      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_push_pbb_header_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case   OFPAT_POP_PBB: /* Pop the outer PBB service tag (I-TAG) */
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPAT_POP_PBB:");
      status=
        ofu_push_pop_pbb_header_action(msg);

      if (status != OFU_ACTION_PUSH_SUCCESS)
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_pop_pbb_header_action failed");
        status=OF_FAILURE;
        break;
      }
      break;
    case OFPAT_EXPERIMENTER:
      {
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"experiment action message");
        switch(action_p->sub_type)
        {
#ifdef FSLX_COMMON_AND_L3_EXT_SUPPORT
          case FSLXAT_WRITE_META_DATA:
            {
              status=fslx_action_write_metadata(msg,action_p->ui64_data);

              if (status != OFU_ACTION_PUSH_SUCCESS)
              {
                OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"fslx_action_write_metadata failed");
                status=OF_FAILURE;
                break;
              }
            }
            break;
          case FSLXAT_IP_FRAGMENT:
            {
              status=fslx_action_ipfragment(msg,action_p->ui32_data);
              if (status != OFU_ACTION_PUSH_SUCCESS)
              {
                OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"fslx_action_ipfragment failed");
                status=OF_FAILURE;
                break;
              }
            }
	    break;
	  case FSLXAT_SEND_ICMPV4_TIME_EXCEED:
	    {
		    status = fslx_action_send_icmpv4_time_exceed_err_msg(msg, action_p->ui8_data);
		    if (status != OFU_ACTION_PUSH_SUCCESS)
		    {
			    OF_LOG_MSG(OF_LOG_FWD_NF, OF_LOG_ERROR,"Error in sending icmp time exceed action  err = %d",status);
			    status = OF_FAILURE;
			    break;
		    }

	    }
	    break;
	  case NXAST_REG_LOAD:
	    {
		    status = nicira_action_write_register(msg, action_p->ui32_data, (uint64_t) action_p->port_no, action_p->ui8_data,(uint8_t)action_p->ui16_data );
		    if(status != OFU_SET_FIELD_SUCCESS)
		    {
			    OF_LOG_MSG(OF_LOG_FWD_NF, OF_LOG_ERROR,"action nicira register 0 write data failed,err = %d",status);
			    status = OF_FAILURE;
			    break;
		    }

	    }
	    break;
	  case FSLXAT_DROP_PACKET:
	    {
		    status = fslx_action_drop_packet(msg);
		    if(status != OFU_ACTION_PUSH_SUCCESS)
		    {
			    OF_LOG_MSG(OF_LOG_FWD_NF, OF_LOG_ERROR,"action drop packet failed,err = %d",status);
			    status = OF_FAILURE;
			    break;
		    }
	    }
	    break;
#endif
	 default:
	    {
		    OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"unknown freescale extention");
		    status=OF_FAILURE;
		    break;
	    }
	}
      }
      break;
    }

     if (status != OFU_ACTION_PUSH_SUCCESS)
     {
         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"ofu_push_output_action failed");
         return OF_FAILURE;
     }

     ofu_end_pushing_actions(msg, &starting_location, &length_of_actions);
     ((struct ofp_bucket*)(msg->desc.ptr2))->len = htons(length_of_actions);
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"length_of_actions=%d",length_of_actions);
     }
    
     *bucket_len = length_of_buckets;

  OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s %d length_of_buckets=%d *bucket_len=%d",__FUNCTION__,__LINE__,length_of_buckets,*bucket_len);
         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s %d",__FUNCTION__,__LINE__);

     return OF_SUCCESS;
}
#endif

int32_t
of_frame_and_send_add_action_info(uint64_t datapath_handle,
                                struct ofi_group_desc_info *group_info,
                                struct ofi_bucket *bucket_info,
                                struct ofi_action *action_info) 
{
    struct of_msg *msg = NULL;
    void *conn_info_p;
    uint16_t  msg_len; 
    uint16_t length_of_buckets=0;
    int32_t status = OF_SUCCESS;
    int32_t retval = OF_SUCCESS;

         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s",__FUNCTION__);
    /* Creating group with one bucket with actions OUTPUT to Port */
    msg_len = OFU_ADD_OR_MODIFY_OR_DELETE_GROUP_MESSAGE +
              OFU_GROUP_ACTION_BUCKET_LEN +
              OFU_OUTPUT_ACTION_LEN +
              OFU_GROUP_BUCKET_PROP_WEIGHT_LEN;
    do
    {
         msg = ofu_alloc_message(OFPT_GROUP_MOD, msg_len);
         if (msg == NULL)
         {
            OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"OF message alloc error");
            status= OF_FAILURE;
            break;
         }

         if ((group_info != NULL) || (bucket_info != NULL))
         {
           retval = of_frame_actions_and_buckets(msg,group_info, bucket_info, NULL, 
                                               action_info, &length_of_buckets);
           if ( retval != OF_SUCCESS)
           {
               OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR," APpending  of bucket to multicast group failed");
               status=retval;
               break;
           }
         }

         /*Append above created bucket to the group id created earlier for multicast group*/
         retval=of_insert_bucket_to_group(msg, datapath_handle, group_info->group_id,
                                          group_info->group_type, bucket_info->bucket_id,
                                          length_of_buckets,&conn_info_p);

         if ( retval != OFU_INSERT_BUCKET_TO_GROUP_SUCCESS)
         {
             OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"of_insert_bucket_to_group failed");
             status=retval;
             break;
         }

         /*retval =  of_modify_group(msg, datapath_handle, group_info->group_id,
                           group_info->group_type, length_of_buckets, &conn_info_p);
         if ( retval != OFU_MODIFY_GROUP_ENTRY_SUCCESS)
         {
             OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"of_insert_bucket_to_group failed");
             status=retval;
             break;
         }*/

         /* Sending the group  mod message with add command to dp*/
         retval=of_cntrlr_send_request_message(msg, datapath_handle, conn_info_p,NULL, NULL);
         if ( retval != OF_SUCCESS)
         {
             OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR," Sending Add group command failed");
             status=retval;
             break;
         }
    }
    while(0);

    if(status != OF_SUCCESS)
    {
         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s ERROR",__FUNCTION__);
      if (msg)
           msg->desc.free_cbk(msg);
    }

         OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"%s %d",__FUNCTION__,__LINE__);
    return status;
}

/* Group, bucket, bucket properties and actions get function to display */

int32_t of_group_frame_response(char *msg, struct ofi_group_desc_info *group_entry_p, 
                                uint32_t msg_length)
{

  struct ofi_bucket *bucket_entry_p;
  uint32_t bucket_length = 0, read_length=0;
  int32_t return_value;

  OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"frame response msg_length %d", msg_length);

  OF_LIST_INIT(group_entry_p->bucket_list, of_group_free_bucket_entry);

  while (msg_length > 0 )
  {
    OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"msg_length =%d", msg_length);
    bucket_length=0;
    return_value= of_group_frame_bucket_response(msg, group_entry_p->group_type, 
                                                &bucket_entry_p, &bucket_length);
    if ( return_value != OF_SUCCESS )
    {
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"of_group_frame_bucket_response failed");
      return OF_FAILURE;
    }
    OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"bucket_length %d", bucket_length);
    OF_APPEND_NODE_TO_LIST(group_entry_p->bucket_list, bucket_entry_p,OF_BUCKET_LISTNODE_OFFSET);
    read_length += bucket_length;
    msg += bucket_length;
    msg_length -= bucket_length;
    OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"msg_length =%d", msg_length);

  }
  // update total read length
  msg_length = read_length;
  return OF_SUCCESS;
}


int32_t of_group_frame_bucket_response(char *msg, uint8_t group_type,  
                                       struct ofi_bucket **bucket_entry_pp, 
                                       uint32_t *msg_length)
{
  struct ofi_bucket *bucket_entry_p;
  struct ofp_bucket *bucket_read_p;
  struct ofi_action *action_entry_p=NULL;
  int32_t total_actions_len=0, total_bucket_prop_len=0;
  uint32_t length=0;
  int32_t return_value;
  int32_t status=OF_SUCCESS;
  struct ofi_bucket_prop *bucket_prop_entry_p;

  OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"Entered");

  bucket_read_p=( struct ofp_bucket *)msg;
  bucket_entry_p=(struct ofi_bucket *)calloc(1,sizeof(struct ofi_bucket ));
  if ( bucket_entry_p == NULL )
  {
    OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"Entered");
    status=OF_FAILURE;
    return status;
  }
  bucket_entry_p->bucket_id = ntohl(bucket_read_p->bucket_id);

  OF_LIST_INIT(bucket_entry_p->action_list, of_group_free_action_entry);
  OF_LIST_INIT(bucket_entry_p->bucket_prop_list, of_group_free_bucket_prop_entry);

  total_actions_len= ntohs(bucket_read_p->action_list_len);

    msg += sizeof(struct ofp_bucket);
    *msg_length += sizeof(struct ofp_bucket);   

  OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"bucket len %d bucketsize %d actionsen %d", 
                         ntohs(bucket_read_p->len), sizeof(struct ofp_bucket), total_actions_len);
  while (total_actions_len > 0 )
  {
    length=0;
    return_value=of_group_frame_action_response(msg, &action_entry_p, &length);
    if ( return_value != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"frame action response failed");
      free(bucket_entry_p); // free bucket entry
      return OF_FAILURE;
    }
    OF_APPEND_NODE_TO_LIST(bucket_entry_p->action_list, action_entry_p, OF_ACTION_LISTNODE_OFFSET);
    total_actions_len -= length;
    msg += length;
    *msg_length += length;
    OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"action_length %d", length);
  }

  total_bucket_prop_len= ntohs(bucket_read_p->len) - (sizeof(struct ofp_bucket)+ntohs(bucket_read_p->action_list_len));

  OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"total_bucket_prop_len=%d", total_bucket_prop_len);
  while (total_bucket_prop_len > 0)
  {
    length=0;
    return_value = of_group_frame_bucket_prop_response(msg, &bucket_prop_entry_p, &length);
    if ( return_value != OF_SUCCESS)
    {
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_ERROR,"frame action response failed");
      free(bucket_entry_p); //free bucket entry
      return OF_FAILURE;
    }
    OF_APPEND_NODE_TO_LIST(bucket_entry_p->bucket_prop_list, bucket_prop_entry_p, OF_BUCKET_PROP_LISTNODE_OFFSET);
    total_bucket_prop_len -= length;
    msg += length;
    *msg_length += length;
    OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"action_length %d", length);
  }

  OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"msg_length=%d", *msg_length);
  *bucket_entry_pp=bucket_entry_p;
  return OF_SUCCESS;
}

int32_t of_group_frame_bucket_prop_response(char *msg, 
                         struct ofi_bucket_prop **bucket_prop_entry_pp, 
                         uint32_t *length)
{
  struct ofi_bucket_prop *bucket_prop_entry_p;
  struct ofp_group_bucket_prop_header *bucket_prop_read_p;
  uint16_t prop_type;

  OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"Entered");

  bucket_prop_read_p = (struct ofp_group_bucket_prop_header *)msg;
  bucket_prop_entry_p=(struct ofi_bucket_prop *)calloc(1,sizeof(struct ofi_bucket_prop ));
  prop_type = ntohs(bucket_prop_read_p->type);

  OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"property type=%d",prop_type);
  switch(prop_type)
  {

    case OFPGBPT_WATCH_PORT:  /* watch port. */
    case OFPGBPT_WATCH_GROUP:  /* watch port. */
      {
        struct ofp_group_bucket_prop_watch *watch_prop;
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPGBPT_WATCH_PORT:OFPGBPT_WATCH_GROUP");
        watch_prop = (struct ofp_group_bucket_prop_watch*)msg;
        bucket_prop_entry_p->watch_group = ntohl(watch_prop->watch);
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"watch= %d \r\n",bucket_prop_entry_p->watch_group, ntohl(watch_prop->watch));
        *length +=  sizeof(struct ofp_group_bucket_prop_watch);
      }
      break;

    case   OFPGBPT_WEIGHT: /* property weight */
      {
        struct ofp_group_bucket_prop_watch *weight_prop;
        weight_prop = (struct ofp_group_bucket_prop_weight*)msg;
        OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"OFPGBPT_WEIGHT");
        bucket_prop_entry_p->weight = ntohs(weight_prop->watch);
        *length +=  sizeof(struct ofp_group_bucket_prop_weight);
      }
      break;

    default:
      {
	 OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,"unknown bucket property %d", ntohs(bucket_prop_read_p->type));
	 *length += ntohs(bucket_prop_read_p->length); 
      }
	break;
  }

  *bucket_prop_entry_pp=bucket_prop_entry_p;
  return OF_SUCCESS;

}

void of_group_free_bucket_entry(struct rcu_head *node)
{
  struct ofi_bucket  *bucket;

  if(node)
  {
    bucket = (struct ofi_bucket *)(((char *)node) - OF_BUCKET_LISTNODE_OFFSET);
    if(bucket)
    {
      free(bucket);
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,
                 "Deleted succesfully group bucket node");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,
                 "Failed to free group bucket node");
    }
  }
  return;
}

void of_group_free_action_entry(struct rcu_head *node)
{
  struct ofi_action  *action;

  if(node)
  {
    action = (struct ofi_action *)(((char *)node) - OF_ACTION_LISTNODE_OFFSET);
    if(action)
    {
      free(action);
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,
                 "Deleted succesfully group action node");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,
                 "Failed to free group action node");
    }
  }
  return;
}

void of_group_free_bucket_prop_entry(struct rcu_head *node)
{
  struct ofi_bucket_prop  *bucket_prop;

  if(node)
  {
    bucket_prop = (struct ofi_bucket_prop *)(((char *)node) - OF_BUCKET_PROP_LISTNODE_OFFSET);
    if(bucket_prop)
    {
      free(bucket_prop);
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,
                 "Deleted succesfully group bucket_prop node");
    }
    else
    {
      OF_LOG_MSG(OF_LOG_MOD, OF_LOG_DEBUG,
                 "Failed to free group bucket_prop node");
    }
  }
  return;
}

