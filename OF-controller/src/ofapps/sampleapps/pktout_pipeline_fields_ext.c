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

/*File: pktout_pipeline_fields_ext.c
 * Description:
 * This application is  used as test case for EXT 427. This extensions 
 * purpose is to send pipeline fields received by contrroler back to switch  
 *
 * Basically giving support in PKT OUT message to carry pipeline fields.
 *
 * This will take metadta pipeline field as example
 *
 * This is not a realistic good  example, this application just return back the
 * pipeline fields recieved. It basically send IN_PORT and META DATA fields
 * back. It only used as test case for EXT 427
 *
 * For TTP details refer to pktout_pipepline_fields_ext_ttp.h  
 * 
 * Code is hard coded to use port numbers attached to datapath as 1 and 2. 
 * 
 * This application just sends packets recieved on port 1 to Port2 and vice versa
 * And uses mata-data to remember port, here metadata is really not required,
 * but just using to test EXT-427.
 *
 * This application pushes two proactive flow entries to table 0 once datapath connected 
 * one for each IN_PORT.
 * Match fields INPORT as 1 or 2 Eth Type as 0x800(IP) Protocol as ICMP
 * Instructions 
 *  1. Write Meta data with a value PORT1_META_DATA in case INPORT as 1
 *                                  PORT2_META_DATA in case INPORT as 2 
 *  2. Appply Actions with one action as OUTPUT to Controller Port
 *
 *  Along with above two entries one additional flow entry will pushed to table 0
 *  Match fileds INPORT as Controller Port Eth Type as 0x800(IP) Protocol as ICMP
 * 
 *  Insturctions
 *      GOTO_TABLE with table id as table 1
 *
 *      Means packet received from Controller will hit this entry and send it to
 *      Table 1.
 *
 * In a similar way  two proactive flow entries to table 1 
 *  Match fields METADATA  as PORT1_META_DATA or PORT2_META_DATA one for each entry
 * Instructions
 * One APPLY Action Instruction for each meta data.
 * If metadata is PORT1_META_DATA, APPLY Action Insturction contains OUTPUT to Port 1
 * If metadata is PORT2_META_DATA, APPLY Action Insturction contains OUTPUT to Port 2
 *
 * Packet IN handler just sends back INPORT and metadata values received back to 
 * datapath. The action as part of PACKET_OUT messges is output to OFPP_TABLE port.
 * 
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include "controller.h"
#include "of_utilities.h"
#include "of_multipart.h"
#include "of_msgapi.h"
#include "cntrl_queue.h"
#include "cntlr_event.h"
#include "cntlr_tls.h"
#include "cntlr_transprt.h"
#include "dprmldef.h"
#include "dprm.h"
#include "fsl_ext.h"
#include "oflog.h"
#include "pktmbuf.h"

#define OF_OXM_HDR_LEN (4)

#define PORT1_META_DATA 0x123A
#define PORT2_META_DATA 0x245B

#define PORT1 1
#define PORT2 2

struct dprm_distributed_forwarding_domain_info  ext427_fwd_domain_info;

void ext427_app_domain_event_cbk(uint32_t notification_type,
                                 uint64_t domain_handle,
                                 struct   dprm_domain_notification_data notification_data,
                                 void     *callback_arg1,
                                 void     *callback_arg2);

int32_t
ext427_app_dp_ready_event(uint64_t  controller_handle,
                          uint64_t   domain_handle,
                          uint64_t   datapath_handle,
                          void       *cbk_arg1,
                          void       *cbk_arg2);

int32_t
ext427_table0_pkt_in_rcvd(int64_t controller_handle,
                          uint64_t domain_handle,
                          uint64_t datapath_handle,
                          struct of_msg *msg,
                          struct ofl_packet_in  *pkt_in,
                          void *cbk_arg1,
                          void *cbk_arg2);
int32_t 
ext427_add_proactive_flow_4_sending_pkt_2_controller(uint64_t datapath_handle, 
                                                     uint32_t in_port);

int32_t
ext427_add_proactive_flow_4_sending_pkt_2_next_table(uint64_t datapath_handle);

int32_t
ext427_add_proactive_flow_4_sending_pkt_2_other_port(uint64_t datapath_handle,
                                                     uint32_t  port_meta_data);

void ext427_app_init(void)
{

   strcpy(ext427_fwd_domain_info.name,"EXT_427_DOMAIN");
   strcpy(ext427_fwd_domain_info.expected_subject_name_for_authentication,"fwd_domain@abc.com");

   if (dprm_register_forwarding_domain_notifications(DPRM_DOMAIN_ALL_NOTIFICATIONS,
                                                     ext427_app_domain_event_cbk,
                                                     &ext427_fwd_domain_info, NULL) != OF_SUCCESS)
   {
       printf("Registering callback to forward domain failed....\r\n");
       return;
   }
}

void ext427_app_domain_event_cbk(uint32_t notification_type,
                                 uint64_t domain_handle,
                                 struct   dprm_domain_notification_data notification_data,
                                 void     *callback_arg1,
                                 void     *callback_arg2)
{
  struct dprm_distributed_forwarding_domain_info  *domain_info=
                    (struct dprm_distributed_forwarding_domain_info *)callback_arg1 ;

  /* TAP_DOMAIN or L3_FWD_DOMAIN belongs to "SAMPLE_L3_FWD_TTP"*/
  if(strcmp(notification_data.domain_name,"EXT_427_DOMAIN"))
  {
    printf("Notification is  not for Pktout Pipeline fields ext\r\n");
    return;
  }

  switch(notification_type)
  {
    case DPRM_DOMAIN_ADDED:
      {
#if 0
         if (strcmp(notification_data.domain_name,"XXXXXX") == 0)
         {
         }
#endif
          if (cntlr_register_asynchronous_event_hook(domain_handle,
                                                   DP_READY_EVENT,
                                                   1,
                                                   ext427_app_dp_ready_event,
                                                   NULL, NULL) == OF_FAILURE)
            {
               printf( "Register DP Ready event cbk failed for EXT_427_DOMAIN ");
            }

            if (of_register_asynchronous_message_hook(domain_handle,
                  0,/*Table 0*/
                  OF_ASYNC_MSG_PACKET_IN_EVENT,
                  2,
                  ext427_table0_pkt_in_rcvd,NULL,NULL) != OF_SUCCESS)
            {
              printf("registering table 0 pktin cbk faield");
            }
      }
      break;
   }
}

int32_t
ext427_app_dp_ready_event(uint64_t  controller_handle,
                          uint64_t   domain_handle,
                          uint64_t   datapath_handle,
                          void       *cbk_arg1,
                          void       *cbk_arg2)
{
  int32_t retval = OF_SUCCESS;
 
  retval = ext427_add_proactive_flow_4_sending_pkt_2_controller(datapath_handle,PORT1);
  {
     if(retval != OF_SUCCESS)
     {
       printf("%s %d Error in programing datpath table 0 for sending pkt(from port 1) to controller\r\n",
        __FUNCTION__,__LINE__);
        return OF_FAILURE;
     }
  }
  retval = ext427_add_proactive_flow_4_sending_pkt_2_controller(datapath_handle,PORT2);
  {
     if(retval != OF_SUCCESS)
     {
       printf("%s %d Error in programing datpath table 0 for sending pkt(from port 2) to controller\r\n",
        __FUNCTION__,__LINE__);
        return OF_FAILURE;
     }
  }

  retval = ext427_add_proactive_flow_4_sending_pkt_2_next_table(datapath_handle);
  {
     if(retval != OF_SUCCESS)
     {
        printf("%s %d Error in programing datpath table 0 for sending  next table \r\n",
        __FUNCTION__,__LINE__);
        return OF_FAILURE;
     }
  }

  retval = ext427_add_proactive_flow_4_sending_pkt_2_other_port(datapath_handle,PORT1_META_DATA);
  {
     if(retval != OF_SUCCESS)
     {
        printf("%s %d Error in programing datpath table 1for sending pkt(to port 2)\r\n",
        __FUNCTION__,__LINE__);
        return OF_FAILURE;
     }
  }

  retval = ext427_add_proactive_flow_4_sending_pkt_2_other_port(datapath_handle,PORT2_META_DATA);
  {
     if(retval != OF_SUCCESS)
     {
        printf("%s %d Error in programing datpath table 1for sending pkt(to port 1)\r\n",
        __FUNCTION__,__LINE__);
        return OF_FAILURE;
     }
  }

  return OF_SUCCESS;
}

int32_t 
ext427_add_proactive_flow_4_sending_pkt_2_controller(uint64_t datapath_handle, 
                                                     uint32_t in_port)
{
  struct      of_msg *msg;
  uint16_t    msg_len;
  uint16_t    instruction_len;
  uint8_t     *inst_start_loc = NULL;
  uint16_t    action_len;
  uint8_t     *action_start_loc = NULL;
  uint16_t    match_fd_len=0;
  uint8_t     *match_start_loc = NULL;
  uint64_t    meta_data;
  uint64_t    meta_data_mask;
  uint16_t    flags=0;
  int32_t     retval = OF_SUCCESS;
  int32_t     status = OF_SUCCESS;
  void *conn_info_p;

  msg_len = (OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+
             OFU_IN_PORT_FIELD_LEN+
             OFU_ETH_TYPE_FIELD_LEN+
             OFU_IP_PROTO_FIELD_LEN+
             OFU_WRITE_META_DATA_INSTRUCTION_LEN+  
             OFU_APPLY_ACTION_INSTRUCTION_LEN+
                      OFU_OUTPUT_ACTION_LEN);

  do
  {
     msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
     if (msg == NULL)
     {
        printf("%s:%d OF message alloc error \r\n",__FUNCTION__,__LINE__);
        status = OF_FAILURE;
        break; 
    }

    flags |= OFPFF_RESET_COUNTS;
    retval = of_create_add_flow_entry_msg(msg,
        datapath_handle,
        0,/*Table Id 0*/
        (10 + in_port), /*priority*/
        OFPFC_ADD,
        0, /*coockie*/
        0,/*Coockie amsk*/
        OFP_NO_BUFFER,
        flags,
        0, /*No Hard timeout*/
        0, /* No Idle timeout*/
        &conn_info_p
        );
    if (retval != OFU_ADD_FLOW_ENTRY_TO_TABLE_SUCCESS)
    {
      printf("Error in adding arp flow entry  = %d", retval);
      status = OF_FAILURE;
      break;
    }

    /* Adding inport field as match field*/
    ofu_start_setting_match_field_values(msg);
    retval = of_set_in_port(msg,&in_port);
    if (retval != OFU_SET_FIELD_SUCCESS)
    {
      printf("%s:%d OF Set In Port field failed,err = %d\r\n",__FUNCTION__,__LINE__,retval);
      status = OF_FAILURE;
      break;
    }

    uint16_t eth_type = 0x0800; /*Eth type is IP*/
    retval = of_set_ether_type(msg,&eth_type);
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      printf("OF Set Eth type as IPV4 failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    uint8_t protocol=0x01; /*protocol as ICMP*/
    retval = of_set_protocol(msg, &protocol);/*it is mandatory to set before setting ports*/
    if (retval != OFU_SET_FIELD_SUCCESS)
    {
      printf("OF Set IP Protocol cols as ICMP failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }
    ofu_end_setting_match_field_values(msg,match_start_loc,&match_fd_len);

    /*Pushing two instructions  one as writing inport number in meta data
      and another apply actions with action as output to controller port
    */
    ofu_start_pushing_instructions(msg, match_fd_len);

    /* Hardcoded logic, If inport as 1 metadta contains 2*/
    if(in_port == PORT1)
       meta_data = (uint64_t)PORT1_META_DATA;
    else if(in_port == PORT2)
       meta_data = (uint64_t)PORT2_META_DATA;
    else
    {
       printf("Unspported port number %d\r\n",in_port);
       status = OF_FAILURE;
       break;
    }
    meta_data_mask = (uint64_t)0xffffffff;
    retval = ofu_push_write_meta_data_instruction(msg, meta_data, meta_data_mask);
    if (retval != OFU_INSTRUCTION_PUSH_SUCCESS)
    {
        printf(" write meta data instruction failed,err = %d",retval);
        status = OF_FAILURE;
        break;
    }

    retval = ofu_start_pushing_apply_action_instruction(msg);
    if (retval != OFU_INSTRUCTION_PUSH_SUCCESS)
    {
       printf("%s:%d  Error in adding apply action instruction err = %d\r\n",
           __FUNCTION__,__LINE__,retval);
       status = OF_FAILURE;
       break;
    }

    retval = ofu_push_output_action(msg, OFPP_CONTROLLER, OFPCML_NO_BUFFER);
    if (retval != OFU_ACTION_PUSH_SUCCESS)
    {
        printf("%s:%d  Error in adding output action  err = %d\r\n",
        __FUNCTION__,__LINE__,retval);
        status = OF_FAILURE;
        break;
    }
    ofu_end_pushing_actions(msg,action_start_loc,&action_len);
    ((struct ofp_instruction_actions*)(msg->desc.ptr2))->len = htons(action_len);

    printf("total actions len = %d for this instruction", action_len);

    ofu_end_pushing_instructions(msg,inst_start_loc,&instruction_len);

    printf("instruction len = %d", instruction_len);

    retval = of_cntrlr_send_request_message(msg, datapath_handle, conn_info_p, NULL, NULL);
    if (retval != OF_SUCCESS)   {
      printf(" Error adding flow entry in route table = %d",retval);
      status = OF_FAILURE;
      break;
    }
  }
  while(0);

  if (status == OF_FAILURE)     {
            msg->desc.free_cbk(msg);
  }

  return status;
}

int32_t 
ext427_add_proactive_flow_4_sending_pkt_2_next_table(uint64_t datapath_handle)
{
  struct      of_msg *msg;
  uint16_t    msg_len;
  uint16_t    instruction_len;
  uint8_t     *inst_start_loc = NULL;
  uint16_t    match_fd_len=0;
  uint8_t     *match_start_loc = NULL;
  uint16_t    flags=0;
  uint32_t    port_num;
  int32_t     retval = OF_SUCCESS;
  int32_t     status = OF_SUCCESS;
  void *conn_info_p;

  msg_len = (OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+
             OFU_ETH_TYPE_FIELD_LEN+
             OFU_IN_PORT_FIELD_LEN+
             OFU_IP_PROTO_FIELD_LEN+
             OFU_GOTO_TABLE_INSTRUCTION_LEN); 

  printf("Msg Len %d\r\n",msg_len);
  do
  {
     msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
     if (msg == NULL)
     {
        printf("%s:%d OF message alloc error \r\n",__FUNCTION__,__LINE__);
        status = OF_FAILURE;
        break; 
    }

    flags |= OFPFF_RESET_COUNTS;
    retval = of_create_add_flow_entry_msg(msg,
        datapath_handle,
        0,/*Table Id 0*/
        (30), /*priority*/
        OFPFC_ADD,
        0, /*coockie*/
        0,/*Coockie amsk*/
        OFP_NO_BUFFER,
        flags,
        0, /*No Hard timeout*/
        0, /* No Idle timeout*/
        &conn_info_p
        );
    if (retval != OFU_ADD_FLOW_ENTRY_TO_TABLE_SUCCESS)
    {
      printf("Error in adding arp flow entry  = %d", retval);
      status = OF_FAILURE;
      break;
    }

    /* Adding inport field as match field, here input port values is controller*/
    ofu_start_setting_match_field_values(msg);
    port_num = OFPP_CONTROLLER;
    retval = of_set_in_port(msg,&port_num);
    if (retval != OFU_SET_FIELD_SUCCESS)
    {
      printf("%s:%d OF Set In Port field failed,err = %d\r\n"
             ,__FUNCTION__,__LINE__,retval);
      status = OF_FAILURE;
      break;
    }
    uint16_t eth_type = 0x0800; /*Eth type is IP*/
    retval = of_set_ether_type(msg,&eth_type);
    if(retval != OFU_SET_FIELD_SUCCESS)
    {
      printf("OF Set Eth type as IPV4 failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }

    uint8_t protocol=0x01; /*protocol as ICMP*/
    retval = of_set_protocol(msg, &protocol);/*it is mandatory to set before setting ports*/
    if (retval != OFU_SET_FIELD_SUCCESS)
    {
      printf("OF Set IP Protocol cols as ICMP failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }


    ofu_end_setting_match_field_values(msg,match_start_loc,&match_fd_len);

    /*Pushing one instructions, in case of pkt from controller send pkt
      next table, that is 2*/ 
    ofu_start_pushing_instructions(msg, match_fd_len);
    retval = ofu_push_go_to_table_instruction(msg,1);
    if (retval != OFU_INSTRUCTION_PUSH_SUCCESS)
    {
        printf("%s:%d go to table instruction failed err = %d\r\n",__FUNCTION__,__LINE__,retval);
        status = OF_FAILURE;
        break;
    }
    ofu_end_pushing_instructions(msg,inst_start_loc,&instruction_len);

    printf("instruction len = %d", instruction_len);

    retval = of_cntrlr_send_request_message(msg, datapath_handle, conn_info_p, NULL, NULL);
    if (retval != OF_SUCCESS)   {
      printf("Error adding flow entry in route table = %d",retval);
      status = OF_FAILURE;
      break;
    }
   }
   while(0);

   if (status == OF_FAILURE)     {
            msg->desc.free_cbk(msg);
  }

  return status;
}

/*If paccket received on port 1, it will send to port  2 and vice versa.
  Here Metadata is used as match field it contains port numumber on which
  pkt is received
*/
int32_t
ext427_add_proactive_flow_4_sending_pkt_2_other_port(uint64_t datapath_handle,
                                                     uint32_t  port_meta_data)
{
  struct      of_msg *msg;
  uint16_t    msg_len;
  uint16_t    instruction_len;
  uint8_t     *inst_start_loc = NULL;
  uint16_t    action_len;
  uint8_t     *action_start_loc = NULL;
  uint16_t    match_fd_len=0;
  uint8_t     *match_start_loc = NULL;
  uint64_t    meta_data;
  uint64_t    meta_data_mask;
  uint16_t    flags=0;
  uint16_t    priority;
  uint32_t    out_port_num;
  int32_t     retval = OF_SUCCESS;
  int32_t     status = OF_SUCCESS;
  void *conn_info_p;

  msg_len = (OFU_ADD_OR_MODIFY_OR_DELETE_FLOW_ENTRY_MESSAGE_LEN+16+
             OFU_META_DATA_FIELD_LEN+
             OFU_APPLY_ACTION_INSTRUCTION_LEN+
                      OFU_OUTPUT_ACTION_LEN);

  printf("Msg Len %d\r\n",msg_len);
  do
  {
     /* Hardcoded logic, If inport as 1 output port is 2*/
    if(port_meta_data == PORT1_META_DATA)
    {
       priority     =(10+PORT1);
       out_port_num = PORT2;
    }
    else if(port_meta_data == PORT2_META_DATA)
    {
       priority     =(10+PORT2);
       out_port_num = PORT1;
    }
    else

    {
       printf("Unspported metadata %x\r\n",port_meta_data);
       status = OF_FAILURE;
       break;
    }


     msg = ofu_alloc_message(OFPT_FLOW_MOD, msg_len);
     if (msg == NULL)
     {
        printf("%s:%d OF message alloc error \r\n",__FUNCTION__,__LINE__);
        status = OF_FAILURE;
        break; 
    }

    flags |= OFPFF_RESET_COUNTS;
    retval = of_create_add_flow_entry_msg(msg,
        datapath_handle,
        1,/*Table Id 0*/
        (priority), /*priority*/
        OFPFC_ADD,
        0, /*coockie*/
        0,/*Coockie amsk*/
        OFP_NO_BUFFER,
        flags,
        0, /*No Hard timeout*/
        0, /* No Idle timeout*/
        &conn_info_p
        );
    if (retval != OFU_ADD_FLOW_ENTRY_TO_TABLE_SUCCESS)
    {
      printf("Error in adding arp flow entry  = %d", retval);
      status = OF_FAILURE;
      break;
    }

    /* Adding inport field as match field, here input port values is controller*/
    ofu_start_setting_match_field_values(msg);
    meta_data = (uint64_t)port_meta_data;
    meta_data_mask = (uint64_t)0xffffffff;
    retval = of_set_meta_data(msg, &meta_data, TRUE, &meta_data_mask);
    if (retval != OFU_SET_FIELD_SUCCESS)
    {
      printf("OF Set meta data failed,err = %d",retval);
      status = OF_FAILURE;
      break;
    }
    ofu_end_setting_match_field_values(msg,match_start_loc,&match_fd_len);

    /*Pushing one instructions, contains one instruction apply actions immeditly
      contains one action called output port */ 
    ofu_start_pushing_instructions(msg, match_fd_len);
 
    retval = ofu_start_pushing_apply_action_instruction(msg);
    if (retval != OFU_INSTRUCTION_PUSH_SUCCESS)
    {
       printf("%s:%d  Error in adding apply action instruction err = %d\r\n",
           __FUNCTION__,__LINE__,retval);
       status = OF_FAILURE;
       break;
    }

    retval = ofu_push_output_action(msg, out_port_num, OFPCML_NO_BUFFER);
    if (retval != OFU_ACTION_PUSH_SUCCESS)
    {
        printf("%s:%d  Error in adding output action  err = %d\r\n",
        __FUNCTION__,__LINE__,retval);
        status = OF_FAILURE;
        break;
    }
    ofu_end_pushing_actions(msg,action_start_loc,&action_len);
    ((struct ofp_instruction_actions*)(msg->desc.ptr2))->len = htons(action_len);

    printf("total actions len = %d for this instruction", action_len);

    ofu_end_pushing_instructions(msg,inst_start_loc,&instruction_len);

    printf("instruction len = %d", instruction_len);

    retval = of_cntrlr_send_request_message(msg, datapath_handle, conn_info_p, NULL, NULL);
    if (retval != OF_SUCCESS)   {
      printf("Error adding flow entry in route table = %d",retval);
      status = OF_FAILURE;
      break;
    }

   }
   while(0);

   if (status == OF_FAILURE)     {
            msg->desc.free_cbk(msg);
  }

  return status;
}

/* Pakcet IN handler */
int32_t
ext427_table0_pkt_in_rcvd(int64_t controller_handle,
                          uint64_t domain_handle,
                          uint64_t datapath_handle,
                          struct of_msg *msg,
                          struct ofl_packet_in  *pkt_in,
                          void *cbk_arg1,
                          void *cbk_arg2)
{
  uint8_t *starting_location;
  uint16_t length;
  uint16_t msg_len;
  uint16_t    length_of_actions;
  uint8_t     *action_start_loc = NULL;
  struct of_msg *reply_msg;
  void          *conn_info_p;
  int32_t retval = OF_SUCCESS;
  int32_t status = OF_SUCCESS;

  msg_len = 
  (OFU_PACKET_OUT_MESSAGE_LEN+16+OFU_OUTPUT_ACTION_LEN+ 
  pkt_in->packet_length + pkt_in->match_fields_length);

  do
  {
     reply_msg=ofu_alloc_message(OFPT_PACKET_OUT, msg_len);
     if(reply_msg == NULL)
     {
       printf("Pkt out message buffer allocation failed");
       status = OF_FAILURE;
       break;
     }

#if 0
     /* We wanted Inport pipeline field as controller port*/
     retval = ofu_set_in_port_field_as_controller(msg,
                                                  pkt_in->match_fields_length,  
                                                  (void*)pkt_in->match_fields);
     if(retval != OFU_SET_FIELD_SUCCESS)
     {
       printf("Set Inport to Controler port is failed");
       status = OF_FAILURE;
       break;
     }
#endif
     retval=of_frame_pktout_msg(
        reply_msg,
        datapath_handle,
        TRUE,
        pkt_in->buffer_id,
        pkt_in->channel_no,
        &conn_info_p
        );
     if (retval != OF_SUCCESS)
     {
       printf(" Error in framing pkt out");
       status = OF_FAILURE;
       break;
     }

     {
        uint32_t in_port = OFPP_CONTROLLER; /*Setting Inport as controller */
        uint64_t meta_data;
        uint8_t *match_start_loc;
        uint16_t  length;
      

        retval = ofu_get_metadata_field(msg, pkt_in->match_fields,
                                     pkt_in->match_fields_length, &meta_data);
        if (retval != OFU_IN_META_DATA_PRESENT)
        {
           printf("%s:failed get metadata value from PktIn ",__FUNCTION__);
           status = OF_FAILURE;
           break;
        }

        printf("Meta data %llx\r\n",meta_data);

        ofu_start_setting_match_field_values(reply_msg);
        of_set_in_port(reply_msg,&in_port);
        of_set_meta_data(reply_msg,&meta_data,FALSE,NULL);
        ofu_end_setting_match_field_values(reply_msg,match_start_loc,&length);
     }

     ofu_start_pushing_actions(reply_msg);
     retval = ofu_push_output_action(reply_msg,OFPP_TABLE,OFPCML_NO_BUFFER);
     if (retval != OFU_ACTION_PUSH_SUCCESS)
     {
         printf("%s:%d  Error in adding output action  err = %d\r\n",
                 __FUNCTION__,__LINE__,retval);
         status = OF_FAILURE;
         break;
     }
     ofu_end_pushing_actions(reply_msg,starting_location,&length_of_actions);
     ((struct ofp_packet_out*)(reply_msg->desc.start_of_data))->actions_len=htons(length_of_actions);

     retval = of_add_data_to_pkt_and_send_to_dp(reply_msg,
                                                 datapath_handle,
                                                 conn_info_p,
                                                 pkt_in->packet_length,
                                                 pkt_in->packet,
                                                 NULL,NULL);
      if (retval != OF_SUCCESS)
      {
          printf("Error in sending packet out to datapath");
          status = OF_FAILURE;
          break;
      }
  }
  while(0);
  
  if(status != OF_SUCCESS)
  {
     if(reply_msg != NULL)
      reply_msg->desc.free_cbk(reply_msg);
  }
  return status;
}
