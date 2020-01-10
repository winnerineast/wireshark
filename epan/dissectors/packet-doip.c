/* packet-doip.c
 * Routines for DoIP (ISO13400) protocol packet disassembly
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include <epan/packet.h>
#include <epan/dissectors/packet-tcp.h>

void proto_register_doip(void);
void proto_reg_handoff_doip(void);



#define DOIP_PORT                                  13400


#define DOIP_GENERIC_NACK                          0x0000
#define DOIP_VEHICLE_IDENTIFICATION_REQ            0x0001
#define DOIP_VEHICLE_IDENTIFICATION_REQ_EID        0x0002
#define DOIP_VEHICLE_IDENTIFICATION_REQ_VIN        0x0003
#define DOIP_VEHICLE_ANNOUNCEMENT_MESSAGE          0x0004
#define DOIP_ROUTING_ACTIVATION_REQUEST            0x0005
#define DOIP_ROUTING_ACTIVATION_RESPONSE           0x0006
#define DOIP_ALIVE_CHECK_REQUEST                   0x0007
#define DOIP_ALIVE_CHECK_RESPONSE                  0x0008
#define DOIP_ENTITY_STATUS_REQUEST                 0x4001
#define DOIP_ENTITY_STATUS_RESPONSE                0x4002
#define DOIP_POWER_INFORMATION_REQUEST             0x4003
#define DOIP_POWER_INFORMATION_RESPONSE            0x4004
#define DOIP_DIAGNOSTIC_MESSAGE                    0x8001
#define DOIP_DIAGNOSTIC_MESSAGE_ACK                0x8002
#define DOIP_DIAGNOSTIC_MESSAGE_NACK               0x8003


/* Header */
#define DOIP_VERSION_OFFSET                        0
#define DOIP_VERSION_LEN                           1
#define DOIP_INV_VERSION_OFFSET                    (DOIP_VERSION_OFFSET + DOIP_VERSION_LEN)
#define DOIP_INV_VERSION_LEN                       1
#define DOIP_TYPE_OFFSET                           (DOIP_INV_VERSION_OFFSET + DOIP_INV_VERSION_LEN)
#define DOIP_TYPE_LEN                              2
#define DOIP_LENGTH_OFFSET                         (DOIP_TYPE_OFFSET + DOIP_TYPE_LEN)
#define DOIP_LENGTH_LEN                            4
#define DOIP_HEADER_LEN                            (DOIP_LENGTH_OFFSET + DOIP_LENGTH_LEN)

#define RESERVED_VER                               0x00
#define ISO13400_2010                              0x01
#define ISO13400_2012                              0x02
#define DEFAULT_VALUE                              0xFF


/* Generic NACK */
#define DOIP_GENERIC_NACK_OFFSET                   DOIP_HEADER_LEN
#define DOIP_GENERIC_NACK_LEN                      1


/* Common */
#define DOIP_COMMON_VIN_LEN                        17
#define DOIP_COMMON_EID_LEN                        6


/*  Vehicle identifcation request */
#define DOIP_VEHICLE_IDENTIFICATION_EID_OFFSET     DOIP_HEADER_LEN
#define DOIP_VEHICLE_IDENTIFICATION_VIN_OFFSET     DOIP_HEADER_LEN


/* Routing activation request */
#define DOIP_ROUTING_ACTIVATION_REQ_SRC_OFFSET     DOIP_HEADER_LEN
#define DOIP_ROUTING_ACTIVATION_REQ_SRC_LEN        2
#define DOIP_ROUTING_ACTIVATION_REQ_TYPE_OFFSET    (DOIP_ROUTING_ACTIVATION_REQ_SRC_OFFSET + DOIP_ROUTING_ACTIVATION_REQ_SRC_LEN)
#define DOIP_ROUTING_ACTIVATION_REQ_TYPE_LEN_V1    2
#define DOIP_ROUTING_ACTIVATION_REQ_TYPE_LEN_V2    1
#define DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V1  (DOIP_ROUTING_ACTIVATION_REQ_TYPE_OFFSET + DOIP_ROUTING_ACTIVATION_REQ_TYPE_LEN_V1)
#define DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2  (DOIP_ROUTING_ACTIVATION_REQ_TYPE_OFFSET + DOIP_ROUTING_ACTIVATION_REQ_TYPE_LEN_V2)
#define DOIP_ROUTING_ACTIVATION_REQ_ISO_LEN        4
#define DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V1  (DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V1 + DOIP_ROUTING_ACTIVATION_REQ_ISO_LEN)
#define DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V2  (DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2 + DOIP_ROUTING_ACTIVATION_REQ_ISO_LEN)
#define DOIP_ROUTING_ACTIVATION_REQ_OEM_LEN        4


/* Routing activation response */
#define DOIP_ROUTING_ACTIVATION_RES_TESTER_OFFSET  DOIP_HEADER_LEN
#define DOIP_ROUTING_ACTIVATION_RES_TESTER_LEN     2
#define DOIP_ROUTING_ACTIVATION_RES_ENTITY_OFFSET  (DOIP_ROUTING_ACTIVATION_RES_TESTER_OFFSET + DOIP_ROUTING_ACTIVATION_RES_TESTER_LEN)
#define DOIP_ROUTING_ACTIVATION_RES_ENTITY_LEN     2
#define DOIP_ROUTING_ACTIVATION_RES_CODE_OFFSET    (DOIP_ROUTING_ACTIVATION_RES_ENTITY_OFFSET + DOIP_ROUTING_ACTIVATION_RES_ENTITY_LEN)
#define DOIP_ROUTING_ACTIVATION_RES_CODE_LEN       1
#define DOIP_ROUTING_ACTIVATION_RES_ISO_OFFSET     (DOIP_ROUTING_ACTIVATION_RES_CODE_OFFSET + DOIP_ROUTING_ACTIVATION_RES_CODE_LEN)
#define DOIP_ROUTING_ACTIVATION_RES_ISO_LEN        4
#define DOIP_ROUTING_ACTIVATION_RES_OEM_OFFSET     (DOIP_ROUTING_ACTIVATION_RES_ISO_OFFSET + DOIP_ROUTING_ACTIVATION_RES_ISO_LEN)
#define DOIP_ROUTING_ACTIVATION_RES_OEM_LEN        4


/* Vehicle announcement message */
#define DOIP_VEHICLE_ANNOUNCEMENT_VIN_OFFSET       DOIP_HEADER_LEN
#define DOIP_VEHICLE_ANNOUNCEMENT_ADDRESS_OFFSET   (DOIP_VEHICLE_ANNOUNCEMENT_VIN_OFFSET + DOIP_COMMON_VIN_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_ADDRESS_LEN      2
#define DOIP_VEHICLE_ANNOUNCEMENT_EID_OFFSET       (DOIP_VEHICLE_ANNOUNCEMENT_ADDRESS_OFFSET + DOIP_VEHICLE_ANNOUNCEMENT_ADDRESS_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_GID_OFFSET       (DOIP_VEHICLE_ANNOUNCEMENT_EID_OFFSET + DOIP_COMMON_EID_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_GID_LEN          6
#define DOIP_VEHICLE_ANNOUNCEMENT_ACTION_OFFSET    (DOIP_VEHICLE_ANNOUNCEMENT_GID_OFFSET + DOIP_VEHICLE_ANNOUNCEMENT_GID_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_ACTION_LEN       1
#define DOIP_VEHICLE_ANNOUNCEMENT_SYNC_OFFSET      (DOIP_VEHICLE_ANNOUNCEMENT_ACTION_OFFSET + DOIP_VEHICLE_ANNOUNCEMENT_ACTION_LEN)
#define DOIP_VEHICLE_ANNOUNCEMENT_SYNC_LEN         1


/* Alive check response */
#define DOIP_ALIVE_CHECK_RESPONSE_SOURCE_OFFSET    DOIP_HEADER_LEN
#define DOIP_ALIVE_CHECK_RESPONSE_SOURCE_LEN       2


/* Entity status response */
#define DOIP_ENTITY_STATUS_RESPONSE_NODE_OFFSET    DOIP_HEADER_LEN
#define DOIP_ENTITY_STATUS_RESPONSE_NODE_LEN       1
#define DOIP_ENTITY_STATUS_RESPONSE_MCTS_OFFSET    (DOIP_ENTITY_STATUS_RESPONSE_NODE_OFFSET + DOIP_ENTITY_STATUS_RESPONSE_NODE_LEN)
#define DOIP_ENTITY_STATUS_RESPONSE_MCTS_LEN       1
#define DOIP_ENTITY_STATUS_RESPONSE_NCTS_OFFSET    (DOIP_ENTITY_STATUS_RESPONSE_MCTS_OFFSET + DOIP_ENTITY_STATUS_RESPONSE_MCTS_LEN)
#define DOIP_ENTITY_STATUS_RESPONSE_NCTS_LEN       1
#define DOIP_ENTITY_STATUS_RESPONSE_MDS_OFFSET     (DOIP_ENTITY_STATUS_RESPONSE_NCTS_OFFSET + DOIP_ENTITY_STATUS_RESPONSE_NCTS_LEN)
#define DOIP_ENTITY_STATUS_RESPONSE_MDS_LEN        4


/* Diagnostic power mode information response */
#define DOIP_POWER_MODE_OFFSET                     DOIP_HEADER_LEN
#define DOIP_POWER_MODE_LEN                        1


/* Common */
#define DOIP_DIAG_COMMON_SOURCE_OFFSET             DOIP_HEADER_LEN
#define DOIP_DIAG_COMMON_SOURCE_LEN                2
#define DOIP_DIAG_COMMON_TARGET_OFFSET             (DOIP_DIAG_COMMON_SOURCE_OFFSET + DOIP_DIAG_COMMON_SOURCE_LEN)
#define DOIP_DIAG_COMMON_TARGET_LEN                2


/* Diagnostic message */
#define DOIP_DIAG_MESSAGE_DATA_OFFSET              (DOIP_DIAG_COMMON_TARGET_OFFSET + DOIP_DIAG_COMMON_TARGET_LEN)


/* Diagnostic message ACK */
#define DOIP_DIAG_MESSAGE_ACK_CODE_OFFSET          (DOIP_DIAG_COMMON_TARGET_OFFSET + DOIP_DIAG_COMMON_TARGET_LEN)
#define DOIP_DIAG_MESSAGE_ACK_CODE_LEN             1
#define DOIP_DIAG_MESSAGE_ACK_PREVIOUS_OFFSET      (DOIP_DIAG_MESSAGE_ACK_CODE_OFFSET + DOIP_DIAG_MESSAGE_ACK_CODE_LEN)


/* Diagnostic message NACK */
#define DOIP_DIAG_MESSAGE_NACK_CODE_OFFSET         (DOIP_DIAG_COMMON_TARGET_OFFSET + DOIP_DIAG_COMMON_TARGET_LEN)
#define DOIP_DIAG_MESSAGE_NACK_CODE_LEN            1
#define DOIP_DIAG_MESSAGE_NACK_PREVIOUS_OFFSET     (DOIP_DIAG_MESSAGE_NACK_CODE_OFFSET + DOIP_DIAG_MESSAGE_NACK_CODE_LEN)



/*
 * Enums
 */

/* Header */
/* Protocol version */
static const value_string doip_versions[] = {
    { RESERVED_VER,  "Reserved" },
    { ISO13400_2010, "DoIP ISO/DIS 13400-2:2010" },
    { ISO13400_2012, "DoIP ISO 13400-2:2012" },
    { DEFAULT_VALUE, "Default value for vehicle identification request messages" },
    { 0, NULL }
};

/* Payload type */
static const value_string doip_payloads[] = {
    { DOIP_GENERIC_NACK,                    "Generic DoIP header NACK" },
    { DOIP_VEHICLE_IDENTIFICATION_REQ,      "Vehicle identification request" },
    { DOIP_VEHICLE_IDENTIFICATION_REQ_EID,  "Vehicle identification request with EID" },
    { DOIP_VEHICLE_IDENTIFICATION_REQ_VIN,  "Vehicle identification request with VIN" },
    { DOIP_VEHICLE_ANNOUNCEMENT_MESSAGE,     "Vehicle announcement message/vehicle identification response message" },
    { DOIP_ROUTING_ACTIVATION_REQUEST, "Routing activation request" },
    { DOIP_ROUTING_ACTIVATION_RESPONSE, "Routing activation response" },
    { DOIP_ALIVE_CHECK_REQUEST, "Alive check request" },
    { DOIP_ALIVE_CHECK_RESPONSE, "Alive check response" },
    { DOIP_ENTITY_STATUS_REQUEST, "DoIP entity status request" },
    { DOIP_ENTITY_STATUS_RESPONSE, "DoIP entity status response" },
    { DOIP_POWER_INFORMATION_REQUEST, "Diagnostic power mode information request" },
    { DOIP_POWER_INFORMATION_RESPONSE, "Diagnostic power mode information response" },
    { DOIP_DIAGNOSTIC_MESSAGE, "Diagnostic message" },
    { DOIP_DIAGNOSTIC_MESSAGE_ACK, "Diagnostic message ACK" },
    { DOIP_DIAGNOSTIC_MESSAGE_NACK, "Diagnostic message NACK" },
    { 0, NULL }
};


/* Generic NACK */
static const value_string nack_codes[] = {
    { 0x00, "Incorrect pattern format" },
    { 0x01, "Unknown payload type" },
    { 0x02, "Message too large" },
    { 0x03, "Out of memory" },
    { 0x04, "Invalid payload length" },
    { 0, NULL }
};


/* Routing activation request */
static const value_string activation_types[] = {
    { 0x00, "Default" },
    { 0x01, "WWH-OBD" },
    { 0xE0, "Central security" },
    { 0, NULL }
};


/* Routing activation response */
static const value_string activation_codes[] = {
    { 0x00, "Routing activation denied due to unknown source address." },
    { 0x01, "Routing activation denied because all concurrently supported TCP_DATA sockets are registered and active." },
    { 0x02, "Routing activation denied because an SA different from the table connection entry was received on the already activated TCP_DATA socket." },
    { 0x03, "Routing activation denied because the SA is already registered and active on a different TCP_DATA socket." },
    { 0x04, "Routing activation denied due to missing authentication." },
    { 0x05, "Routing activation denied due to rejected confirmation." },
    { 0x06, "Routing activation denied due to unsupported routing activation type." },
    { 0x07, "Reserved by ISO 13400." },
    { 0x08, "Reserved by ISO 13400." },
    { 0x09, "Reserved by ISO 13400." },
    { 0x0A, "Reserved by ISO 13400." },
    { 0x0B, "Reserved by ISO 13400." },
    { 0x0C, "Reserved by ISO 13400." },
    { 0x0D, "Reserved by ISO 13400." },
    { 0x0E, "Reserved by ISO 13400." },
    { 0x0F, "Reserved by ISO 13400." },
    { 0x10, "Routing successfully activated." },
    { 0x11, "Routing will be activated; confirmation required." },
    { 0, NULL }
};


/* Vehicle announcement message */
/* Action code */
static const value_string action_codes[] = {
    { 0x00, "No further action required" },
    { 0x01, "Reserved by ISO 13400" },
    { 0x02, "Reserved by ISO 13400" },
    { 0x03, "Reserved by ISO 13400" },
    { 0x04, "Reserved by ISO 13400" },
    { 0x05, "Reserved by ISO 13400" },
    { 0x06, "Reserved by ISO 13400" },
    { 0x07, "Reserved by ISO 13400" },
    { 0x08, "Reserved by ISO 13400" },
    { 0x09, "Reserved by ISO 13400" },
    { 0x0A, "Reserved by ISO 13400" },
    { 0x0B, "Reserved by ISO 13400" },
    { 0x0C, "Reserved by ISO 13400" },
    { 0x0D, "Reserved by ISO 13400" },
    { 0x0E, "Reserved by ISO 13400" },
    { 0x0F, "Reserved by ISO 13400" },
    { 0x10, "Routing activation required to initiate central security" },
    { 0, NULL }
};

/* Sync status */
static const value_string sync_status[] = {
    { 0x00, "VIN and/or GID are synchronized" },
    { 0x01, "Reserved by ISO 13400" },
    { 0x02, "Reserved by ISO 13400" },
    { 0x03, "Reserved by ISO 13400" },
    { 0x04, "Reserved by ISO 13400" },
    { 0x05, "Reserved by ISO 13400" },
    { 0x06, "Reserved by ISO 13400" },
    { 0x07, "Reserved by ISO 13400" },
    { 0x08, "Reserved by ISO 13400" },
    { 0x09, "Reserved by ISO 13400" },
    { 0x0A, "Reserved by ISO 13400" },
    { 0x0B, "Reserved by ISO 13400" },
    { 0x0C, "Reserved by ISO 13400" },
    { 0x0D, "Reserved by ISO 13400" },
    { 0x0E, "Reserved by ISO 13400" },
    { 0x0F, "Reserved by ISO 13400" },
    { 0x10, "Incomplete: VIN and GID are NOT synchronized" },
    { 0, NULL }
};

/* Entity status response */
/* Node type */
static const value_string node_types[] = {
    { 0x00, "DoIP gateway" },
    { 0x01, "DoIp node" },
    { 0, NULL }
};


/* Diagnostic power mode information response */
/* Power mode */
static const value_string power_modes[] = {
    { 0x00, "not ready" },
    { 0x01, "ready" },
    { 0x02, "not supported" },
    { 0, NULL }
};


/* Diagnostic message ACK */
static const value_string diag_ack_codes[] = {
    { 0x00, "ACK" },
    { 0, NULL }
};


/* Diagnostic message NACK */
static const value_string diag_nack_codes[] = {
    { 0x00, "Reserved by ISO 13400" },
    { 0x01, "Reserved by ISO 13400" },
    { 0x02, "Invalid source address" },
    { 0x03, "Unknown target address" },
    { 0x04, "Diagnostic message too large" },
    { 0x05, "Out of memory" },
    { 0x06, "Target unreachable" },
    { 0x07, "Unknown network" },
    { 0x08, "Transport protocol error" },
    { 0, NULL }
};



/*
 * Fields
 */

/* DoIP header */
static int hf_doip_version = -1;
static int hf_doip_inv_version = -1;
static int hf_doip_type = -1;
static int hf_doip_length = -1;


/* Generic NACK */
static int hf_generic_nack_code = -1;


/* Common */
static int hf_reserved_iso = -1;
static int hf_reserved_oem = -1;


/* Routing activation request */
static int hf_activation_type_v1 = -1;
static int hf_activation_type_v2 = -1;


/* Routing activation response */
static int hf_tester_logical_address = -1;
static int hf_response_code = -1;


/* Vehicle announcement message */
static int hf_logical_address = -1;
static int hf_gid = -1;
static int hf_futher_action = -1;
static int hf_sync_status = -1;


/* Diagnostic power mode information response */
static int hf_power_mode = -1;


/* Entity status response */
static int hf_node_type = -1;
static int hf_max_sockets = -1;
static int hf_current_sockets = -1;
static int hf_max_data_size = -1;


/* Common */
static int hf_vin = -1;
static int hf_eid = -1;
static int hf_source_address = -1;
static int hf_target_address = -1;
static int hf_previous = -1;


/* Diagnostic message */
static int hf_data = -1;


/* Diagnostic message ACK */
static int hf_ack_code = -1;


/* Diagnostic message NACK */
static int hf_nack_code = -1;



/*
 * Trees
 */
static gint ett_doip = -1;

/* DoIP header */
static gint ett_header = -1;


/* Misc */
static dissector_handle_t doip_handle;
static dissector_handle_t uds_handle;
static gint proto_doip    = -1;



static void
add_header(proto_tree *doip_tree, tvbuff_t *tvb)
{
    proto_tree *subtree = proto_tree_add_subtree(doip_tree, tvb, DOIP_VERSION_OFFSET, DOIP_HEADER_LEN, ett_header, NULL, "Header");
    proto_tree_add_item(subtree, hf_doip_version, tvb, DOIP_VERSION_OFFSET, DOIP_VERSION_LEN, ENC_BIG_ENDIAN);
    proto_tree_add_item(subtree, hf_doip_inv_version, tvb, DOIP_INV_VERSION_OFFSET, DOIP_INV_VERSION_LEN, ENC_BIG_ENDIAN);
    proto_tree_add_item(subtree, hf_doip_type, tvb, DOIP_TYPE_OFFSET, DOIP_TYPE_LEN, ENC_BIG_ENDIAN);
    proto_tree_add_item(subtree, hf_doip_length, tvb, DOIP_LENGTH_OFFSET, DOIP_LENGTH_LEN, ENC_BIG_ENDIAN);
}


static void
add_generic_header_nack_fields(proto_tree *doip_tree, tvbuff_t *tvb)
{
    proto_tree_add_item(doip_tree, hf_generic_nack_code, tvb, DOIP_GENERIC_NACK_OFFSET, DOIP_GENERIC_NACK_LEN, ENC_NA);
}


static void
add_vehicle_identification_eid_fields(proto_tree *doip_tree, tvbuff_t *tvb)
{
    proto_tree_add_item(doip_tree, hf_eid, tvb, DOIP_VEHICLE_IDENTIFICATION_EID_OFFSET, DOIP_COMMON_EID_LEN, ENC_NA);
}


static void
add_vehicle_identification_vin_fields(proto_tree *doip_tree, tvbuff_t *tvb)
{
    proto_tree_add_item(doip_tree, hf_vin, tvb, DOIP_VEHICLE_IDENTIFICATION_VIN_OFFSET, DOIP_COMMON_VIN_LEN, ENC_ASCII | ENC_NA);
}


static void
add_routing_activation_request_fields(proto_tree *doip_tree, tvbuff_t *tvb, guint8 version)
{
    proto_tree_add_item(doip_tree, hf_source_address, tvb, DOIP_ROUTING_ACTIVATION_REQ_SRC_OFFSET, DOIP_ROUTING_ACTIVATION_REQ_SRC_LEN, ENC_BIG_ENDIAN);

    if (version == ISO13400_2010) {
        proto_tree_add_item(doip_tree, hf_activation_type_v1, tvb, DOIP_ROUTING_ACTIVATION_REQ_TYPE_OFFSET, DOIP_ROUTING_ACTIVATION_REQ_TYPE_LEN_V1, ENC_NA);
        proto_tree_add_item(doip_tree, hf_reserved_iso, tvb, DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V1, DOIP_ROUTING_ACTIVATION_REQ_ISO_LEN, ENC_BIG_ENDIAN);

        if ( tvb_bytes_exist(tvb, DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V1, DOIP_ROUTING_ACTIVATION_REQ_OEM_LEN) ) {
            proto_tree_add_item(doip_tree, hf_reserved_oem, tvb, DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V1, DOIP_ROUTING_ACTIVATION_REQ_OEM_LEN, ENC_BIG_ENDIAN);
        }
    } else if (version == ISO13400_2012) {
        proto_tree_add_item(doip_tree, hf_activation_type_v2, tvb, DOIP_ROUTING_ACTIVATION_REQ_TYPE_OFFSET, DOIP_ROUTING_ACTIVATION_REQ_TYPE_LEN_V2, ENC_NA);
        proto_tree_add_item(doip_tree, hf_reserved_iso, tvb, DOIP_ROUTING_ACTIVATION_REQ_ISO_OFFSET_V2, DOIP_ROUTING_ACTIVATION_REQ_ISO_LEN, ENC_BIG_ENDIAN);

        if ( tvb_bytes_exist(tvb, DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V2, DOIP_ROUTING_ACTIVATION_REQ_OEM_LEN) ) {
            proto_tree_add_item(doip_tree, hf_reserved_oem, tvb, DOIP_ROUTING_ACTIVATION_REQ_OEM_OFFSET_V2, DOIP_ROUTING_ACTIVATION_REQ_OEM_LEN, ENC_BIG_ENDIAN);
        }
    }
}


static void
add_routing_activation_response_fields(proto_tree *doip_tree, tvbuff_t *tvb)
{
    proto_tree_add_item(doip_tree, hf_tester_logical_address, tvb, DOIP_ROUTING_ACTIVATION_RES_TESTER_OFFSET, DOIP_ROUTING_ACTIVATION_RES_TESTER_LEN, ENC_BIG_ENDIAN);
    proto_tree_add_item(doip_tree, hf_source_address, tvb, DOIP_ROUTING_ACTIVATION_RES_ENTITY_OFFSET, DOIP_ROUTING_ACTIVATION_RES_ENTITY_LEN, ENC_BIG_ENDIAN);
    proto_tree_add_item(doip_tree, hf_response_code, tvb, DOIP_ROUTING_ACTIVATION_RES_CODE_OFFSET, DOIP_ROUTING_ACTIVATION_RES_CODE_LEN, ENC_NA);
    proto_tree_add_item(doip_tree, hf_reserved_iso, tvb, DOIP_ROUTING_ACTIVATION_RES_ISO_OFFSET, DOIP_ROUTING_ACTIVATION_RES_ISO_LEN, ENC_BIG_ENDIAN);

    if ( tvb_bytes_exist(tvb, DOIP_ROUTING_ACTIVATION_RES_OEM_OFFSET, DOIP_ROUTING_ACTIVATION_RES_OEM_LEN) ) {
        proto_tree_add_item(doip_tree, hf_reserved_oem, tvb, DOIP_ROUTING_ACTIVATION_RES_OEM_OFFSET, DOIP_ROUTING_ACTIVATION_RES_OEM_LEN, ENC_BIG_ENDIAN);
    }
}


static void
add_vehicle_announcement_message_fields(proto_tree *doip_tree, tvbuff_t *tvb)
{
    proto_tree_add_item(doip_tree, hf_vin, tvb, DOIP_VEHICLE_ANNOUNCEMENT_VIN_OFFSET, DOIP_COMMON_VIN_LEN, ENC_ASCII | ENC_NA);
    proto_tree_add_item(doip_tree, hf_logical_address, tvb, DOIP_VEHICLE_ANNOUNCEMENT_ADDRESS_OFFSET, DOIP_VEHICLE_ANNOUNCEMENT_ADDRESS_LEN, ENC_BIG_ENDIAN);
    proto_tree_add_item(doip_tree, hf_eid, tvb, DOIP_VEHICLE_ANNOUNCEMENT_EID_OFFSET, DOIP_COMMON_EID_LEN, ENC_NA);
    proto_tree_add_item(doip_tree, hf_gid, tvb, DOIP_VEHICLE_ANNOUNCEMENT_GID_OFFSET, DOIP_VEHICLE_ANNOUNCEMENT_GID_LEN, ENC_NA);
    proto_tree_add_item(doip_tree, hf_futher_action, tvb, DOIP_VEHICLE_ANNOUNCEMENT_ACTION_OFFSET, DOIP_VEHICLE_ANNOUNCEMENT_ACTION_LEN, ENC_BIG_ENDIAN);

    if ( tvb_bytes_exist(tvb, DOIP_VEHICLE_ANNOUNCEMENT_SYNC_OFFSET, DOIP_VEHICLE_ANNOUNCEMENT_SYNC_LEN) ) {
        /* Not part of version 1 and optional in version 2. */
        proto_tree_add_item(doip_tree, hf_sync_status, tvb, DOIP_VEHICLE_ANNOUNCEMENT_SYNC_OFFSET, DOIP_VEHICLE_ANNOUNCEMENT_SYNC_LEN, ENC_BIG_ENDIAN);
    }
}


static void
add_alive_check_response_fields(proto_tree *doip_tree, tvbuff_t *tvb)
{
    proto_tree_add_item(doip_tree, hf_source_address, tvb, DOIP_ALIVE_CHECK_RESPONSE_SOURCE_OFFSET, DOIP_ALIVE_CHECK_RESPONSE_SOURCE_LEN, ENC_BIG_ENDIAN);
}


static void
add_entity_status_response_fields(proto_tree *doip_tree, tvbuff_t *tvb)
{
    proto_tree_add_item(doip_tree, hf_node_type, tvb, DOIP_ENTITY_STATUS_RESPONSE_NODE_OFFSET, DOIP_ENTITY_STATUS_RESPONSE_NODE_LEN, ENC_NA);
    proto_tree_add_item(doip_tree, hf_max_sockets, tvb, DOIP_ENTITY_STATUS_RESPONSE_MCTS_OFFSET, DOIP_ENTITY_STATUS_RESPONSE_MCTS_LEN, ENC_NA);
    proto_tree_add_item(doip_tree, hf_current_sockets, tvb, DOIP_ENTITY_STATUS_RESPONSE_NCTS_OFFSET, DOIP_ENTITY_STATUS_RESPONSE_NCTS_LEN, ENC_NA);
    if ( tvb_bytes_exist(tvb, DOIP_ENTITY_STATUS_RESPONSE_MDS_OFFSET, DOIP_ENTITY_STATUS_RESPONSE_MDS_LEN) ) {
        proto_tree_add_item(doip_tree, hf_max_data_size, tvb, DOIP_ENTITY_STATUS_RESPONSE_MDS_OFFSET, DOIP_ENTITY_STATUS_RESPONSE_MDS_LEN, ENC_BIG_ENDIAN);
    }
}


static void
add_power_mode_information_response_fields(proto_tree *doip_tree, tvbuff_t *tvb)
{
    proto_tree_add_item(doip_tree, hf_power_mode, tvb, DOIP_POWER_MODE_OFFSET, DOIP_POWER_MODE_LEN, ENC_NA);
}


static void
add_diagnostic_message_fields(proto_tree *doip_tree, tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
    proto_tree_add_item(doip_tree, hf_source_address, tvb, DOIP_DIAG_COMMON_SOURCE_OFFSET, DOIP_DIAG_COMMON_SOURCE_LEN, ENC_BIG_ENDIAN);
    proto_tree_add_item(doip_tree, hf_target_address, tvb, DOIP_DIAG_COMMON_TARGET_OFFSET, DOIP_DIAG_COMMON_TARGET_LEN, ENC_BIG_ENDIAN);

    if (uds_handle != 0) {
        call_dissector(uds_handle, tvb_new_subset_length_caplen(tvb, DOIP_DIAG_MESSAGE_DATA_OFFSET, -1, -1), pinfo, parent_tree);
    }  else if (tvb_reported_length_remaining(tvb, DOIP_DIAG_MESSAGE_DATA_OFFSET) > 0) {
        proto_tree_add_item(doip_tree, hf_data, tvb, DOIP_DIAG_MESSAGE_DATA_OFFSET, tvb_reported_length_remaining(tvb, DOIP_DIAG_MESSAGE_DATA_OFFSET), ENC_NA);
    }
}


static void
add_diagnostic_message_ack_fields(proto_tree *doip_tree, tvbuff_t *tvb)
{
    proto_tree_add_item(doip_tree, hf_source_address, tvb, DOIP_DIAG_COMMON_SOURCE_OFFSET, DOIP_DIAG_COMMON_SOURCE_LEN, ENC_BIG_ENDIAN);
    proto_tree_add_item(doip_tree, hf_target_address, tvb, DOIP_DIAG_COMMON_TARGET_OFFSET, DOIP_DIAG_COMMON_TARGET_LEN, ENC_BIG_ENDIAN);
    proto_tree_add_item(doip_tree, hf_ack_code, tvb, DOIP_DIAG_MESSAGE_ACK_CODE_OFFSET, DOIP_DIAG_MESSAGE_ACK_CODE_LEN, ENC_NA);

    if (tvb_captured_length_remaining(tvb, DOIP_DIAG_MESSAGE_ACK_PREVIOUS_OFFSET) > 0) {
        proto_tree_add_item(doip_tree, hf_previous, tvb, DOIP_DIAG_MESSAGE_ACK_PREVIOUS_OFFSET, tvb_captured_length_remaining(tvb, DOIP_DIAG_MESSAGE_ACK_PREVIOUS_OFFSET), ENC_NA);
    }
}


static void
add_diagnostic_message_nack_fields(proto_tree *doip_tree, tvbuff_t *tvb)
{
    proto_tree_add_item(doip_tree, hf_source_address, tvb, DOIP_DIAG_COMMON_SOURCE_OFFSET, DOIP_DIAG_COMMON_SOURCE_LEN, ENC_BIG_ENDIAN);
    proto_tree_add_item(doip_tree, hf_target_address, tvb, DOIP_DIAG_COMMON_TARGET_OFFSET, DOIP_DIAG_COMMON_TARGET_LEN, ENC_BIG_ENDIAN);
    proto_tree_add_item(doip_tree, hf_nack_code, tvb, DOIP_DIAG_MESSAGE_NACK_CODE_OFFSET, DOIP_DIAG_MESSAGE_NACK_CODE_LEN, ENC_NA);

    if (tvb_captured_length_remaining(tvb, DOIP_DIAG_MESSAGE_NACK_PREVIOUS_OFFSET) > 0) {
        proto_tree_add_item(doip_tree, hf_previous, tvb, DOIP_DIAG_MESSAGE_NACK_PREVIOUS_OFFSET, tvb_captured_length_remaining(tvb, DOIP_DIAG_MESSAGE_NACK_PREVIOUS_OFFSET), ENC_NA);
    }
}


/* DoIP protocol dissector */
static void
dissect_doip_message(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    guint8 version = tvb_get_guint8(tvb, DOIP_VERSION_OFFSET);
    guint16 payload_type = tvb_get_ntohs(tvb, DOIP_TYPE_OFFSET);

    /* Set protocol and clear information columns */
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "DoIP");
    col_clear(pinfo->cinfo, COL_INFO);

    if (
        version == ISO13400_2010 ||
        version == ISO13400_2012 ||
        (version == DEFAULT_VALUE && (payload_type >= DOIP_VEHICLE_IDENTIFICATION_REQ && payload_type <= DOIP_VEHICLE_IDENTIFICATION_REQ_EID))
        ) {
        col_add_fstr(pinfo->cinfo, COL_INFO, "%s", val_to_str(payload_type, doip_payloads, "0x%04x Unknown payload"));
    } else {
        col_set_str(pinfo->cinfo, COL_INFO, "Invalid DoIP version");
        return;
    }


    if (tree) {
        proto_item *ti = NULL;
        proto_tree *doip_tree = NULL;

        ti = proto_tree_add_item(tree, proto_doip, tvb, 0, -1, ENC_NA);
        doip_tree = proto_item_add_subtree(ti, ett_doip);

        add_header(doip_tree, tvb);

        switch (payload_type) {
        case DOIP_GENERIC_NACK:
            add_generic_header_nack_fields(doip_tree, tvb);
            break;

        case DOIP_VEHICLE_IDENTIFICATION_REQ:
            break;

        case DOIP_VEHICLE_IDENTIFICATION_REQ_EID:
            add_vehicle_identification_eid_fields(doip_tree, tvb);
            break;

        case DOIP_VEHICLE_IDENTIFICATION_REQ_VIN:
            add_vehicle_identification_vin_fields(doip_tree, tvb);
            break;

        case DOIP_ROUTING_ACTIVATION_REQUEST:
            add_routing_activation_request_fields(doip_tree, tvb, version);
            break;

        case DOIP_ROUTING_ACTIVATION_RESPONSE:
            add_routing_activation_response_fields(doip_tree, tvb);
            break;

        case DOIP_VEHICLE_ANNOUNCEMENT_MESSAGE:
            add_vehicle_announcement_message_fields(doip_tree, tvb);
            break;

        case DOIP_ALIVE_CHECK_REQUEST:
            break;

        case DOIP_ALIVE_CHECK_RESPONSE:
            add_alive_check_response_fields(doip_tree, tvb);
            break;

        case DOIP_ENTITY_STATUS_REQUEST:
            break;

        case DOIP_ENTITY_STATUS_RESPONSE:
            add_entity_status_response_fields(doip_tree, tvb);
            break;

        case DOIP_POWER_INFORMATION_REQUEST:
            break;

        case DOIP_POWER_INFORMATION_RESPONSE:
            add_power_mode_information_response_fields(doip_tree, tvb);
            break;

        case DOIP_DIAGNOSTIC_MESSAGE:
            add_diagnostic_message_fields(doip_tree, tvb, pinfo, tree);
            break;

        case DOIP_DIAGNOSTIC_MESSAGE_ACK:
            add_diagnostic_message_ack_fields(doip_tree, tvb);
            break;

        case DOIP_DIAGNOSTIC_MESSAGE_NACK:
            add_diagnostic_message_nack_fields(doip_tree, tvb);
            break;
        }
    } else if (payload_type == DOIP_DIAGNOSTIC_MESSAGE) {
        /* Show UDS details in info column */
        if (uds_handle != 0) {
            call_dissector(uds_handle, tvb_new_subset_length_caplen(tvb, DOIP_DIAG_MESSAGE_DATA_OFFSET, -1, -1), pinfo, NULL);
        }
    }
}


/* determine PDU length of protocol DoIP */
static guint
get_doip_message_len(packet_info *pinfo _U_, tvbuff_t *tvb, int offset, void *p _U_)
{
    /* PDU Length = length field value + header length */
    return (guint)tvb_get_ntohl(tvb, offset + DOIP_LENGTH_OFFSET) + DOIP_HEADER_LEN;
}


static int
dissect_doip_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    dissect_doip_message(tvb, pinfo, tree);
    return tvb_captured_length(tvb);
}


static int
dissect_doip(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree _U_, void* data)
{
    tcp_dissect_pdus(tvb, pinfo, tree, TRUE, DOIP_HEADER_LEN, get_doip_message_len, dissect_doip_pdu, data);
    return tvb_captured_length(tvb);
}


/* Register DoIP Protocol */
void
proto_register_doip(void)
{
    static hf_register_info hf[] = {
        /* Header */
        { &hf_doip_version,
          { "Version", "doip.version",
            FT_UINT8, BASE_HEX,
            VALS(doip_versions), 0x0,
            NULL, HFILL }
        },
        { &hf_doip_inv_version,
          { "Inverse version", "doip.inverse",
            FT_UINT8, BASE_HEX,
            NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_doip_type,
          { "Type", "doip.type",
            FT_UINT16, BASE_HEX,
            VALS(doip_payloads), 0x0,
            NULL, HFILL }
        },
        { &hf_doip_length,
          { "Length", "doip.length",
            FT_UINT32, BASE_DEC,
            NULL, 0x0,
            NULL, HFILL }
        },
        /* Generic NACK */
        {
            &hf_generic_nack_code,
            {
                "DoIP Header NACK code", "doip.nack_code",
                FT_UINT8, BASE_HEX,
                VALS(nack_codes), 0x00,
                NULL, HFILL
            }
        },
        /* Vehicle announcement message */
        {
            &hf_vin,
            {
                "VIN", "doip.vin",
                FT_STRING, BASE_NONE,
                NULL, 0x00,
                NULL, HFILL
            }
        },
        {
            &hf_logical_address,
            {
                "Logical Address", "doip.logical_address",
                FT_UINT16, BASE_HEX,
                NULL, 0x00,
                NULL, HFILL
            }
        },
        {
            &hf_eid,
            {
                "EID", "doip.eid",
                FT_BYTES, BASE_NONE,
                NULL, 0x00,
                NULL, HFILL
            }
        },
        {
            &hf_gid,
            {
                "GID", "doip.gid",
                FT_BYTES, BASE_NONE,
                NULL, 0x00,
                NULL, HFILL
            }
        },
        {
            &hf_futher_action,
            {
                "Further action required", "doip.futher_action",
                FT_UINT8, BASE_HEX,
                VALS(action_codes), 0x00,
                NULL, HFILL
            }
        },
        {
            &hf_sync_status,
            {
                "VIN/GID sync. status", "doip.sync_status",
                FT_UINT8, BASE_HEX,
                VALS(sync_status), 0x00,
                NULL, HFILL
            }
        },
        /* Diagnostic power mode information response */
        {
            &hf_power_mode,
            {
                "Diagnostic power mode", "doip.power_mode",
                FT_UINT8, BASE_HEX,
                VALS(power_modes), 0x00,
                NULL, HFILL
            }
        },
        /* Entity status response */
        {
            &hf_node_type,
            {
                "Node type", "doip.node_type",
                FT_UINT8, BASE_HEX,
                VALS(node_types), 0x00,
                NULL, HFILL
            }
        },
        {
            &hf_max_sockets,
            {
                "Max concurrent sockets", "doip.max_sockets",
                FT_UINT8, BASE_DEC,
                NULL, 0x00,
                NULL, HFILL
            }
        },
        {
            &hf_current_sockets,
            {
                "Currently open sockets", "doip.sockets",
                FT_UINT8, BASE_DEC,
                NULL, 0x00,
                NULL, HFILL
            }
        },
        {
            &hf_max_data_size,
            {
                "Max data size", "doip.max_data_size",
                FT_UINT32, BASE_DEC,
                NULL, 0x00,
                NULL, HFILL
            }
        },
        /* Common */
        {
            &hf_source_address,
            {
                "Source Address", "doip.source_address",
                FT_UINT16, BASE_HEX,
                NULL, 0x00,
                NULL, HFILL
            }
        },
        {
            &hf_target_address,
            {
                "Target Address", "doip.target_address",
                FT_UINT16, BASE_HEX,
                NULL, 0x00,
                NULL, HFILL
            }
        },
        /* Routing activation request */
        {
            &hf_activation_type_v1,
            {
                "Activation type", "doip.activation_type_v1",
                FT_UINT16, BASE_HEX,
                NULL, 0x00,
                NULL, HFILL
            }
        },
        {
            &hf_activation_type_v2,
            {
                "Activation type", "doip.activation_type",
                FT_UINT8, BASE_HEX,
                VALS(activation_types), 0x00,
                NULL, HFILL
            }
        },
        /* Routing activation response */
        {
            &hf_tester_logical_address,
            {
                "Logical address of external tester", "doip.tester_logical_address",
                FT_UINT16, BASE_HEX,
                NULL, 0x00,
                NULL, HFILL
            }
        },
        {
            &hf_response_code,
            {
                "Routing activation response code", "doip.response_code",
                FT_UINT8, BASE_HEX,
                VALS(activation_codes), 0x00,
                NULL, HFILL
            }
        },
        /* Common */
        {
            &hf_reserved_iso,
            {
                "Reserved by ISO", "doip.reserved_iso",
                FT_UINT32, BASE_HEX,
                NULL, 0x00,
                NULL, HFILL
            }
        },
        {
            &hf_reserved_oem,
            {
                "Reserved by OEM", "doip.reserved_oem",
                FT_UINT32, BASE_HEX,
                NULL, 0x00,
                NULL, HFILL
            }
        },
        /* Diagnostic message */
        {
            &hf_data,
            {
                "User data", "doip.data",
                FT_BYTES, BASE_NONE,
                NULL, 0x00,
                NULL, HFILL
            }
        },
        /* Diagnostic message ACK */
        {
            &hf_ack_code,
            {
                "ACK code", "doip.diag_ack_code",
                FT_UINT8, BASE_HEX,
                VALS(diag_ack_codes), 0x00,
                NULL, HFILL
            }
        },
        /* Diagnostic message NACK */
        {
            &hf_nack_code,
            {
                "NACK code", "doip.diag_nack_code",
                FT_UINT8, BASE_HEX,
                VALS(diag_nack_codes), 0x00,
                NULL, HFILL
            }
        },
        /* Common */
        {
            &hf_previous,
            {
                "Previous message", "doip.previous",
                FT_BYTES, BASE_NONE,
                NULL, 0x00,
                NULL, HFILL
            }
        }
    };

    /* Setup protocol subtree array */
    static gint *ett[] = {
        &ett_doip,
        &ett_header
    };

    proto_doip = proto_register_protocol (
                                          "DoIP (ISO13400) Protocol", /* name       */
                                          "DoIP",                     /* short name */
                                          "doip"                      /* abbrev     */
                                          );

    proto_register_field_array(proto_doip, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    doip_handle = register_dissector("doip", dissect_doip, proto_doip);
}

void
proto_reg_handoff_doip(void)
{
    dissector_add_uint("udp.port", DOIP_PORT, doip_handle);
    dissector_add_uint("tcp.port", DOIP_PORT, doip_handle);

    uds_handle = find_dissector("uds");
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
