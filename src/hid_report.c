#include "hid_report.h"
#include "usbh_hid.h"
#include "debug.h"

/* local debug level for this module */
static int debuglevel = DBG_INFO;

/*
 * Minimal, non-intrusive utilities to inspect the HID Report Descriptor
 * and log presence of Usage Pages and Report IDs. Does not modify behavior.
 */

/* simple container for discovered info (local static) */
#define MAX_REPORT_IDS 8
static struct {
    uint8_t inited;
    uint8_t has_kbd;
    uint8_t has_consumer;
    uint8_t has_system;
    uint8_t report_ids[MAX_REPORT_IDS];
    uint8_t report_ids_count;
} g_hid_info;

static void hid_info_reset(void)
{
    g_hid_info.inited = 0;
    g_hid_info.has_kbd = 0;
    g_hid_info.has_consumer = 0;
    g_hid_info.has_system = 0;
    g_hid_info.report_ids_count = 0;
    for (int i = 0; i < MAX_REPORT_IDS; ++i) g_hid_info.report_ids[i] = 0;
}

int hid_report_init_for_interface(USBH_HandleTypeDef *phost, uint8_t interface_number)
{
    (void)phost;
    (void)interface_number;
    hid_info_reset();
    return 0; /* success (no-op) */
}

/* very light parser: walks through descriptor items to find Usage Page and Report ID */
int hid_report_parse_descriptor(USBH_HandleTypeDef *phost, uint8_t interface_number)
{
    (void)interface_number; /* single interface typical for keyboards */

    if (phost == NULL || phost->pActiveClass == NULL || phost->pActiveClass->pData == NULL)
        return -1;

    HID_HandleTypeDef *HID_Handle = (HID_HandleTypeDef *)phost->pActiveClass->pData;

    /* Ensure descriptor is available; usbh_hid.c state machine normally fetched it already */
    uint16_t desc_len = HID_Handle->HID_Desc.wItemLength;
    if (desc_len == 0) {
        DBG_W("HID Report Descriptor length is 0\r\n");
        return -1;
    }

    /* The ST driver internally stores the report descriptor in HID_Handle->pData during parsing cycles.
     * However, it's not exposed directly here. We can request it explicitly into our temp buffer. */
    uint16_t to_read = desc_len;
    /* Cap a reasonable max length */
    if (to_read > 512) to_read = 512;

    static uint8_t desc_buf[512];
    if (USBH_HID_GetHIDReportDescriptor(phost, to_read) != USBH_OK) {
        DBG_W("USBH_HID_GetHIDReportDescriptor failed\r\n");
        /* Even if this fails, proceed cautiously; some stacks may forbid re-GET here */
        return -1;
    }

    /* After calling GetHIDReportDescriptor, the core copies into phost->device.Data buffer.
     * Use phost->device.Data with length 'to_read'. */
    const uint8_t *buf = phost->device.Data;
    if (buf == NULL) {
        DBG_W("No buffer for HID report descriptor\r\n");
        return -1;
    }

    hid_info_reset();

    uint16_t i = 0;
    uint16_t usage_page = 0;

    while (i < to_read) {
        uint8_t b = buf[i++];
        uint8_t size_code = b & 0x03;          /* 0,1,2,3 => 0,1,2,4 bytes */
        uint8_t type = (b >> 2) & 0x03;        /* 0=Main,1=Global,2=Local */
        uint8_t tag = (b >> 4) & 0x0F;         /* item tag */
        uint32_t value = 0;
        uint8_t size = (size_code == 3) ? 4 : size_code;
        if (size > 0) {
            if (i + size > to_read) break;
            /* little-endian value assemble */
            for (uint8_t k = 0; k < size; ++k) value |= ((uint32_t)buf[i++]) << (8 * k);
        }

        if (type == 1) { /* Global */
            if (tag == 0x00) { /* USAGE_PAGE */
                usage_page = (uint16_t)value;
                if (usage_page == 0x07) g_hid_info.has_kbd = 1;
                if (usage_page == 0x0C) g_hid_info.has_consumer = 1;
                if (usage_page == 0x01) g_hid_info.has_system = 1; /* Generic Desktop includes System Control */
            } else if (tag == 0x08) { /* REPORT_ID */
                if (g_hid_info.report_ids_count < MAX_REPORT_IDS) {
                    g_hid_info.report_ids[g_hid_info.report_ids_count++] = (uint8_t)(value & 0xFF);
                }
            }
        }
    }

    g_hid_info.inited = 1;

    DBG_I("HID Report Descriptor parsed: KBD=%d, CONSUMER=%d, SYSTEM=%d, ReportIDs=%d\r\n",
          g_hid_info.has_kbd, g_hid_info.has_consumer, g_hid_info.has_system, g_hid_info.report_ids_count);
    if (g_hid_info.report_ids_count > 0) {
        for (uint8_t r = 0; r < g_hid_info.report_ids_count; ++r) {
            DBG_V(" - ReportID[%d]=%u\r\n", r, g_hid_info.report_ids[r]);
        }
    }

    return 0;
}

int hid_report_decode(USBH_HandleTypeDef *phost,
                      uint8_t interface_number,
                      const uint8_t *report,
                      uint16_t report_len,
                      void (*event_cb)(const hid_input_event_t *evt))
{
    (void)phost;
    (void)interface_number;
    (void)report;
    (void)report_len;
    (void)event_cb;
    return -1; /* not implemented yet */
}
