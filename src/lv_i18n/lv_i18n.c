#include "./lv_i18n.h"


////////////////////////////////////////////////////////////////////////////////
// Define plural operands
// http://unicode.org/reports/tr35/tr35-numbers.html#Operands

// Integer version, simplified

#define UNUSED(x) (void)(x)

static inline uint32_t op_n(int32_t val) { return (uint32_t)(val < 0 ? -val : val); }
static inline uint32_t op_i(uint32_t val) { return val; }
// always zero, when decimal part not exists.
static inline uint32_t op_v(uint32_t val) { UNUSED(val); return 0;}
static inline uint32_t op_w(uint32_t val) { UNUSED(val); return 0; }
static inline uint32_t op_f(uint32_t val) { UNUSED(val); return 0; }
static inline uint32_t op_t(uint32_t val) { UNUSED(val); return 0; }

static lv_i18n_phrase_t en_gb_singulars[] = {
    {"heater", "Heater: %s"},
    {"back", "Back"},
    {"open_sub_view", "Open Sub View"},
    {"home", "Home"},
    {"macros", "Macros"},
    {"console", "Console"},
    {"estop", "EStop"},
    {"toollist_tool", "Tool"},
    {"toollist_status", "Status"},
    {"toollist_current", "Current"},
    {"toollist_active", "Active"},
    {"toollist_standby", "Standby"},
    {"default_tool_name", "Tool"},
    {"bed", "Bed"},
    {"chamber", "Chamber"},
    {"active", "Active"},
    {"fault", "Fault"},
    {"off", "Off"},
    {"offline", "Offline"},
    {"standby", "Standby"},
    {"tuning", "Tuning"},
    {"tool_list_numpad_header_tool_heater", "Tool %d Heater %d %s"},
    {"tool_list_numpad_header_tool_spindle", "Tool %d Spindle"},
    {"tool_list_numpad_header_bed_chamber", "%s %d %s"},
    {"move", "Move"},
    {"extrude", "Extrude"},
    {"status", "Status"},
    {"fans", "Fans"},
    {"files", "Files"},
    {"object_cancel", "Object Cancel"},
    {"settings", "Settings"},
    {"settings_debug_level", "Debug Level"},
    {"settings_debug_borders", "Enable Debug Borders"},
    {"settings_developer", "Developer"},
    {"move_axis_home", "Home %s"},
    {"home_all", "Home All"},
    {"true_bed_level", "True Bed Level"},
    {"mesh_bed_level", "Mesh Bed Level"},
    {"heightmap", "Heightmap"},
    {"disable_motors", "Disable Motors"},
    {"move_tool_position", "Tool Pos (mm)"},
    {"move_machine_position", "Machine Pos (mm)"},
    {"move_feedrate", "Feedrate (mm/s)"},
    {"settings_duet", "Duet"},
    {"save", "Save"},
    {"settings_duet_hostname", "IP Address: "},
    {"settings_duet_password", "Password: "},
    {"settings_duet_poll_interval", "Poll Interval (ms)"},
    {"unload", "Unload"},
    {"retract", "Retract"},
    {"extrude_feed_dist", "Feed Distance (mm)"},
    {"extrude_feed_rate", "Feed Rate (mm/s)"},
    {"toollist_filament", "Filament"},
    {"none", "None"},
    {"file_header", "Folder: %s"},
    {"refresh", "Refresh"},
    {"gcode_G0_desc", "Rapid move"},
    {"gcode_G1_desc", "Controlled linear move"},
    {"gcode_M117_desc", "Display Message"},
    {"gcode_M118_desc", "Send Message to Specific Target"},
    {"folder", "Folder"},
    {"file", "File"},
    {"settings_network_ip_address", "IP Address: %s"},
    {"settings_connectivity_header", "Connectivity"},
    {"settings_network", "Network"},
    {"settings_dev_header", "Developer"},
    {"settings_network_ssid", "SSID"},
    {"settings_network_signal", "Signal"},
    {"settings_network_known", "Known"},
    {"settings_network_forget", "Forget"},
    {"settings_network_connected", "Connected"},
    {"settings_network_password_title", "Password"},
    {"settings_network_enter_password", "Enter Password"},
    {"settings_enable_ssh", "Enable SSH"},
    {"settings_network_enable", "Enable WiFi"},
    {"msgbox_cancel", "Cancel"},
    {"msgbox_ok", "Ok"},
    {"open_console", "Open Console"},
    {"msgbox_close", "Close"},
    {"msgbox_warning_int_range", "Value must be between %d and %d"},
    {"msgbox_warning_float_range", "Value must be between %f and %f"},
    {"msgbox_warning_text_length", "Input must be between %d and %d characters"},
    {"file_start_print_title", "Start Print"},
    {"file_start_print_message", "File: %s\nDate: %s\nSize: %s"},
    {"settings_duet_info_timeout", "Notification Timeout (ms)"},
    {"pause", "Pause"},
    {"resume", "Resume"},
    {"cancel", "Cancel"},
    {"print_cancel_title", "Cancel Print"},
    {"print_cancel_message", "Are you sure you want to cancel the print?"},
    {"status_tool_temp", "%.1f/%d"},
    {"status_bed_temp", "%.1f/%d"},
    {"status_speed", "%.1f/%.1f mm/s"},
    {"status_flow_rate", "%.1f mm/s^3"},
    {"status_flow_multiplier", "Flow: %u%%"},
    {"status_speed_multiplier", "Speed: %u%%"},
    {"status_elapsed_time", "Elapsed: %s"},
    {"status_remaining_time", "Remaining: %s"},
    {"status_layer", "Z: %.2f/%.2f mm"},
    {"status_fan_speed", "%u%%"},
    {"print_again", "Print Again"},
    {"file_run_macro_title", "Run Macro"},
    {"file_run_macro_message", "Do you want to run %s?"},
    {"settings_restart", "Restart"},
    {"settings_erase_and_restart", "Erase and Restart"},
    {"settings_reboot", "Reboot"},
    {"fine_tune_babystep_increment", "+"},
    {"fine_tune_babystep_decrement", "-"},
    {"fine_tune_babystep_reset", "Reset %.3fmm"},
    {"fine_tune_speed_factor", "Speed Factor"},
    {"fine_tune_extruder_header", "Extrusion Factor"},
    {"fine_tune_fan_header", "Fans"},
    {"fine_tune_extruder", "Extruder %u"},
    {"fine_tune_fan", "Fan %u"},
    {"fine_tune", "Fine Tune"},
    {"fan_header", "Fans"},
    {"max", "Max"},
    {"fan", "Fan"},
    {"sort_by_name", "Name"},
    {"sort_by_date", "Date"},
    {"sort_by_size", "Size"},
    {"settings_device", "Device"},
    {"settings_brightness", "Brightness"},
    {"settings_screensaver_timeout", "Screensaver Timeout"},
    {"update_available", "Update Available"},
    {"update_available_text", "Confirm to install the update"},
    {"update_confirm", "Update"},
    {"update_cancel", "Cancel"},
    {"settings_system_logging", "Log system messages to console"},
    {"heightmap_unload", "Unload"},
    {"heightmap_load", "Load"},
    {"heightmap_list_header", "Heightmaps"},
    {"heightmap_num_points", "Number of points: %u"},
    {"heightmap_area", "Probe area: %.2f cm^2"},
    {"heightmap_min_error", "Min deviation: %.2f mm"},
    {"heightmap_max_error", "Max deviation: %.2f mm"},
    {"heightmap_mean_error", "Mean error: %.2f mm"},
    {"heightmap_std_dev", "RMS error: %.2f mm"},
    {"heightmap_fixed", "Fixed"},
    {"heightmap_auto", "Auto"},
    {"heightmap_render_mode", "Render Mode:"},
    {"heightmap_title", "Heightmap: %s"},
    {"connected_message", "Connected to Duet"},
    {"disconnected_message", "Disconnected from Duet"},
    {"settings_duet_connection_method", "Connection Method: "},
    {"settings_duet_hostname_prompt", "Enter IP Address"},
    {"settings_duet_password_prompt", "Enter Password"},
    {"usb", "USB"},
    {"network", "Network"},
    {"uart", "UART"},
    {"settings_language", "Language"},
    {"settings_language_en", "English"},
    {"settings_theme", "Theme"},
    {"settings_theme_light", "Light"},
    {"settings_usb_mode", "USB-C Mode"},
    {"settings_usb_mode_auto_detect", "USB-C Auto Detect (not implemented)"},
    {"settings_usb_mode_host", "USB-C Host"},
    {"settings_usb_mode_device", "USB-C Device"},
    {"settings_usb_mode_internal_wifi", "Internal WiFi"},
    {"settings_firmware_version", "Firmware Version: %s"},
    {"settings_build_time", "Firmware built on: %s %s"},
    {NULL, NULL} // End mark
};



static uint8_t en_gb_plural_fn(int32_t num)
{
    uint32_t n = op_n(num); UNUSED(n);
    uint32_t i = op_i(n); UNUSED(i);
    uint32_t v = op_v(n); UNUSED(v);

    if ((i == 1 && v == 0)) return LV_I18N_PLURAL_TYPE_ONE;
    return LV_I18N_PLURAL_TYPE_OTHER;
}

static const lv_i18n_lang_t en_gb_lang = {
    .locale_name = "en-GB",
    .singulars = en_gb_singulars,

    .locale_plural_fn = en_gb_plural_fn
};

const lv_i18n_language_pack_t lv_i18n_language_pack[] = {
    &en_gb_lang,
    NULL // End mark
};

////////////////////////////////////////////////////////////////////////////////


// Internal state
static const lv_i18n_language_pack_t * current_lang_pack;
static const lv_i18n_lang_t * current_lang;


/**
 * Reset internal state. For testing.
 */
void __lv_i18n_reset(void)
{
    current_lang_pack = NULL;
    current_lang = NULL;
}

/**
 * Set the languages for internationalization
 * @param langs pointer to the array of languages. (Last element has to be `NULL`)
 */
int lv_i18n_init(const lv_i18n_language_pack_t * langs)
{
    if(langs == NULL) return -1;
    if(langs[0] == NULL) return -1;

    current_lang_pack = langs;
    current_lang = langs[0];     /*Automatically select the first language*/
    return 0;
}

/**
 * Change the localization (language)
 * @param l_name name of the translation locale to use. E.g. "en-GB"
 */
int lv_i18n_set_locale(const char * l_name)
{
    if(current_lang_pack == NULL) return -1;

    uint16_t i;

    for(i = 0; current_lang_pack[i] != NULL; i++) {
        // Found -> finish
        if(strcmp(current_lang_pack[i]->locale_name, l_name) == 0) {
            current_lang = current_lang_pack[i];
            return 0;
        }
    }

    return -1;
}


static const char * __lv_i18n_get_text_core(lv_i18n_phrase_t * trans, const char * msg_id)
{
    uint16_t i;
    for(i = 0; trans[i].msg_id != NULL; i++) {
        if(strcmp(trans[i].msg_id, msg_id) == 0) {
            /*The msg_id has found. Check the translation*/
            if(trans[i].translation) return trans[i].translation;
        }
    }

    return NULL;
}


/**
 * Get the translation from a message ID
 * @param msg_id message ID
 * @return the translation of `msg_id` on the set local
 */
const char * lv_i18n_get_text(const char * msg_id)
{
    if(current_lang == NULL) return msg_id;

    const lv_i18n_lang_t * lang = current_lang;
    const void * txt;

    // Search in current locale
    if(lang->singulars != NULL) {
        txt = __lv_i18n_get_text_core(lang->singulars, msg_id);
        if (txt != NULL) return txt;
    }

    // Try to fallback
    if(lang == current_lang_pack[0]) return msg_id;
    lang = current_lang_pack[0];

    // Repeat search for default locale
    if(lang->singulars != NULL) {
        txt = __lv_i18n_get_text_core(lang->singulars, msg_id);
        if (txt != NULL) return txt;
    }

    return msg_id;
}

/**
 * Get the translation from a message ID and apply the language's plural rule to get correct form
 * @param msg_id message ID
 * @param num an integer to select the correct plural form
 * @return the translation of `msg_id` on the set local
 */
const char * lv_i18n_get_text_plural(const char * msg_id, int32_t num)
{
    if(current_lang == NULL) return msg_id;

    const lv_i18n_lang_t * lang = current_lang;
    const void * txt;
    lv_i18n_plural_type_t ptype;

    // Search in current locale
    if(lang->locale_plural_fn != NULL) {
        ptype = lang->locale_plural_fn(num);

        if(lang->plurals[ptype] != NULL) {
            txt = __lv_i18n_get_text_core(lang->plurals[ptype], msg_id);
            if (txt != NULL) return txt;
        }
    }

    // Try to fallback
    if(lang == current_lang_pack[0]) return msg_id;
    lang = current_lang_pack[0];

    // Repeat search for default locale
    if(lang->locale_plural_fn != NULL) {
        ptype = lang->locale_plural_fn(num);

        if(lang->plurals[ptype] != NULL) {
            txt = __lv_i18n_get_text_core(lang->plurals[ptype], msg_id);
            if (txt != NULL) return txt;
        }
    }

    return msg_id;
}

/**
 * Get the name of the currently used locale.
 * @return name of the currently used locale. E.g. "en-GB"
 */
const char * lv_i18n_get_current_locale(void)
{
    if(!current_lang) return NULL;
    return current_lang->locale_name;
}
