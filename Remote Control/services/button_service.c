/*
 * The MIT License (MIT)
 * Copyright (c) 2018 Novel Bits
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#include <string.h>

#include "nrf_log.h"
#include "button_service.h"

static const uint8_t ButtonOnCharName[] = "Button ON press";
static const uint8_t ButtonOffCharName[] = "Button OFF press";

/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_bas       Button service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_button_service_t * p_button_service, ble_evt_t const * p_ble_evt)
{
    p_button_service->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_bas       Button service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_button_service_t * p_button_service, ble_evt_t const * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_button_service->conn_handle = BLE_CONN_HANDLE_INVALID;
}

/**@brief Function for handling the Write event.
 *
 * @param[in]   p_button_service  Button Service structure.
 * @param[in]   p_ble_evt         Event received from the BLE stack.
 */
static void on_write(ble_button_service_t * p_button_service, ble_evt_t const * p_ble_evt)
{
    // Only continue if we have an event handler to call
    if (p_button_service->evt_handler == NULL)
    {
        return;
    }

    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    NRF_LOG_INFO("Write event received");

    // Handle the Button Off Notification enabled/disabled event
    if (    (p_evt_write->handle == p_button_service->button_off_press_char_handles.cccd_handle)
        &&  (p_evt_write->len == 2))
    {
        if (p_button_service->evt_handler == NULL)
        {
            NRF_LOG_INFO("Event handler is NULL");
            return;
        }

        ble_button_evt_t evt;

        if (ble_srv_is_notification_enabled(p_evt_write->data))
        {
            NRF_LOG_INFO("Button Off Notification enabled");
            evt.evt_type = BLE_BUTTON_OFF_EVT_NOTIFICATION_ENABLED;
        }
        else
        {
            NRF_LOG_INFO("Button Off Notification disabled");
            evt.evt_type = BLE_BUTTON_OFF_EVT_NOTIFICATION_DISABLED;
        }
        evt.conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;

        // CCCD written, call application event handler.
        p_button_service->evt_handler(p_button_service, &evt);
    }
    // Handle the Button On Notification enabled/disabled event
    else if (    (p_evt_write->handle == p_button_service->button_on_press_char_handles.cccd_handle)
        &&  (p_evt_write->len == 2))
    {
        if (p_button_service->evt_handler == NULL)
        {
            NRF_LOG_INFO("Event handler is NULL");
            return;
        }

        ble_button_evt_t evt;

        if (ble_srv_is_notification_enabled(p_evt_write->data))
        {
            NRF_LOG_INFO("Button On Notification enabled");
            evt.evt_type = BLE_BUTTON_ON_EVT_NOTIFICATION_ENABLED;
        }
        else
        {
            NRF_LOG_INFO("Button On Notification disabled");
            evt.evt_type = BLE_BUTTON_ON_EVT_NOTIFICATION_DISABLED;
        }
        evt.conn_handle = p_ble_evt->evt.gatts_evt.conn_handle;

        // CCCD written, call application event handler.
        p_button_service->evt_handler(p_button_service, &evt);
    }
}

/**@brief Function for adding the Button ON press characteristic.
 *
 */
static uint32_t button_on_press_char_add(ble_button_service_t * p_button_service)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));
    memset(&cccd_md, 0, sizeof(cccd_md));
    memset(&attr_md, 0, sizeof(attr_md));
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    // Set permissions on the CCCD and Characteristic value
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);

    // CCCD settings (needed for notifications and/or indications)
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    char_md.char_props.read          = 1;
    char_md.char_props.notify        = 1;
    char_md.p_char_user_desc         = ButtonOnCharName;
    char_md.char_user_desc_size      = sizeof(ButtonOnCharName);
    char_md.char_user_desc_max_size  = sizeof(ButtonOnCharName);
    char_md.p_char_pf                = NULL;
    char_md.p_user_desc_md           = NULL;
    char_md.p_cccd_md                = &cccd_md;
    char_md.p_sccd_md                = NULL;

    // Define the Button ON press Characteristic UUID
    ble_uuid.type = p_button_service->uuid_type;
    ble_uuid.uuid = BLE_UUID_BUTTON_ON_PRESS_CHAR_UUID;

    // Attribute Metadata settings
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    // Attribute Value settings
    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(uint8_t);
    attr_char_value.p_value      = NULL;

    return sd_ble_gatts_characteristic_add(p_button_service->service_handle, &char_md,
                                           &attr_char_value,
                                           &p_button_service->button_on_press_char_handles);
}

/**@brief Function for adding the Button OFF press characteristic.
 *
 */
static uint32_t button_off_press_char_add(ble_button_service_t * p_button_service)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));
    memset(&cccd_md, 0, sizeof(cccd_md));
    memset(&attr_md, 0, sizeof(attr_md));
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    // Set permissions on the CCCD and Characteristic value
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);

    // CCCD settings (needed for notifications and/or indications)
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    char_md.char_props.read          = 1;
    char_md.char_props.notify        = 1;
    char_md.p_char_user_desc         = ButtonOffCharName;
    char_md.char_user_desc_size      = sizeof(ButtonOffCharName);
    char_md.char_user_desc_max_size  = sizeof(ButtonOffCharName);
    char_md.p_char_pf                = NULL;
    char_md.p_user_desc_md           = NULL;
    char_md.p_cccd_md                = &cccd_md;
    char_md.p_sccd_md                = NULL;

    // Define the Button OFF press Characteristic UUID
    ble_uuid.type = p_button_service->uuid_type;
    ble_uuid.uuid = BLE_UUID_BUTTON_OFF_PRESS_CHAR_UUID;

    // Attribute Metadata settings
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;

    // Attribute Value settings
    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(uint8_t);
    attr_char_value.p_value      = NULL;

    return sd_ble_gatts_characteristic_add(p_button_service->service_handle, &char_md,
                                           &attr_char_value,
                                           &p_button_service->button_off_press_char_handles);
}

uint32_t ble_button_service_init(ble_button_service_t * p_button_service, ble_button_evt_handler_t evt_handler)
{
    uint32_t   err_code;
    ble_uuid_t ble_uuid;

    // Initialize service structure
    p_button_service->conn_handle = BLE_CONN_HANDLE_INVALID;
    p_button_service->evt_handler = evt_handler;

    // Add service UUID
    ble_uuid128_t base_uuid = {BLE_UUID_BUTTON_SERVICE_BASE_UUID};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_button_service->uuid_type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Set up the UUID for the service (base + service-specific)
    ble_uuid.type = p_button_service->uuid_type;
    ble_uuid.uuid = BLE_UUID_BUTTON_SERVICE_UUID;

    // Set up and add the service
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_button_service->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add the different characteristics in the service:
    //   ON Button press characteristic:   E54B0002-67F5-479E-8711-B3B99198CE6C
    //   OFF Button press characteristic:  E54B0003-67F5-479E-8711-B3B99198CE6C
    err_code = button_on_press_char_add(p_button_service);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    err_code = button_off_press_char_add(p_button_service);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}

void ble_button_service_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    ble_button_service_t * p_button_service = (ble_button_service_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_button_service, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_button_service, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_button_service, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}

void button_characteristic_update(ble_button_service_t * p_button_service, uint8_t pin_no, uint8_t *button_action, bool button_notifications_enabled)
{
    if (p_button_service->conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        uint32_t err_code;
        uint16_t           len = sizeof(uint8_t);
        ble_gatts_value_t  gatts_value;

        // Initialize value struct.
        memset(&gatts_value, 0, sizeof(gatts_value));

        gatts_value.len     = sizeof(uint8_t);
        gatts_value.offset  = 0;
        gatts_value.p_value = button_action;

        // Update database.
        if (pin_no == BUTTON_1)
        {
            err_code = sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID,
                                              p_button_service->button_on_press_char_handles.value_handle,
                                              &gatts_value);
        }
        else if (pin_no == BUTTON_2)
        {
            err_code = sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID,
                                              p_button_service->button_off_press_char_handles.value_handle,
                                              &gatts_value);
        }

        // Only send a Notification if the Client has subscribed
        if (button_notifications_enabled)
        {
            ble_gatts_hvx_params_t hvx_params;
            memset(&hvx_params, 0, sizeof(hvx_params));

            if (pin_no == BUTTON_1)
            {
                hvx_params.handle = p_button_service->button_on_press_char_handles.value_handle;
            }
            else
            {
                hvx_params.handle = p_button_service->button_off_press_char_handles.value_handle;
            }
            hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
            hvx_params.offset = 0;
            hvx_params.p_len  = &len;
            hvx_params.p_data = (uint8_t*)button_action;

            sd_ble_gatts_hvx(p_button_service->conn_handle, &hvx_params);
        }
    }
}