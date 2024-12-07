#include "panel.h"
#include "../../core/data_setup.h"
#include <stdio.h>
#include "../ui_utils.h"

char time_buffer[12];

char* time_display(unsigned long time){
    unsigned long hours = time / 3600;
    unsigned long minutes = (time % 3600) / 60;
    unsigned long seconds = (time % 3600) % 60;
    sprintf(time_buffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    return time_buffer;
}

static void progress_bar_update(lv_event_t* e){
    lv_obj_t * bar = lv_event_get_target(e);
    lv_bar_set_value(bar, printer.print_progress * 100, LV_ANIM_ON);
}

static void progress_arc_update(lv_event_t* e){
    lv_obj_t * arc = lv_event_get_target(e);
    lv_arc_set_value(arc, printer.print_progress * 360);
}

static void update_printer_data_elapsed_time(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    lv_label_set_text(label, time_display(printer.elapsed_time_s));
}

static void update_printer_data_remaining_time(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    lv_label_set_text(label, time_display(printer.remaining_time_s));
}

static void update_printer_data_stats(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char buff[256] = {0};

    switch (get_current_printer_config()->show_stats_on_progress_panel)
    {
        case SHOW_STATS_ON_PROGRESS_PANEL_LAYER:
            sprintf(buff, "Layer %d of %d", printer.current_layer, printer.total_layers);
            break;
        case SHOW_STATS_ON_PROGRESS_PANEL_PARTIAL:
            sprintf(buff, "Position: X%.2f Y%.2f\nFeedrate: %d mm/s\nFilament Used: %.2f m\nLayer %d of %d", 
            printer.position[0], printer.position[1], printer.feedrate_mm_per_s, printer.filament_used_mm / 1000, printer.current_layer, printer.total_layers);
            break;
        case SHOW_STATS_ON_PROGRESS_PANEL_ALL:
            sprintf(buff, "Pressure Advance: %.3f (%.2fs)\nPosition: X%.2f Y%.2f Z%.2f\nFeedrate: %d mm/s\nFilament Used: %.2f m\nFan: %.0f%%\nSpeed: %.0f%%\nFlow: %.0f%%\nLayer %d of %d", 
            printer.pressure_advance, printer.smooth_time, printer.position[0], printer.position[1], printer.position[2], printer.feedrate_mm_per_s, printer.filament_used_mm / 1000, printer.fan_speed * 100, printer.speed_mult * 100, printer.extrude_mult * 100, printer.current_layer, printer.total_layers);
            break;
    }

    lv_label_set_text(label, buff);
}

static void update_printer_data_percentage(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char percentage_buffer[12];
    sprintf(percentage_buffer, "%.2f%%", printer.print_progress * 100);
    lv_label_set_text(label, percentage_buffer);
}

static void update_printer_layer_printed(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char layer_buffer[12];
    sprintf(layer_buffer, "%d/%d", printer.current_layer , printer.total_layers );
    lv_label_set_text(label, layer_buffer);
}

static void update_printer_hotend(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char layer_buffer[20];
    snprintf(layer_buffer, sizeof(layer_buffer), "%3.1f/%3.0f", printer.extruder_temp, printer.extruder_target_temp);
    lv_label_set_text(label, layer_buffer);
}

static void update_printer_bed(lv_event_t * e){
    lv_obj_t * label = lv_event_get_target(e);
    char layer_buffer[20];
    snprintf(layer_buffer, sizeof(layer_buffer), "%3.1f/%3.0f", printer.bed_temp , printer.bed_target_temp );
    lv_label_set_text(label, layer_buffer);
}

static void btn_click_stop(lv_event_t * e){
    send_gcode(true, "CANCEL_PRINT");
}

static void btn_click_pause(lv_event_t * e){
    send_gcode(true, "PAUSE");
}

static void btn_click_resume(lv_event_t * e){
    send_gcode(true, "RESUME");
}

static void btn_click_estop(lv_event_t * e){
    send_estop();
    send_gcode(false, "M112");
}

void progress_panel_init(lv_obj_t* panel){
    auto panel_width = CYD_SCREEN_PANEL_WIDTH_PX - CYD_SCREEN_GAP_PX * 3;
    const auto button_size_mult = 1.3f;

    // Emergency Stop
    if (global_config.show_estop){
        lv_obj_t * btn = lv_btn_create(panel);
        lv_obj_add_event_cb(btn, btn_click_estop, LV_EVENT_CLICKED, NULL);
        
        lv_obj_set_height(btn, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, CYD_SCREEN_GAP_PX);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF0000), LV_PART_MAIN);

        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, LV_SYMBOL_POWER " EMERGENCY STOP");
        lv_obj_center(label);
    }

    lv_obj_t * center_panel = lv_create_empty_panel(panel);
    lv_obj_set_size(center_panel, panel_width, LV_SIZE_CONTENT);
    lv_layout_flex_column(center_panel);

    // Only align progress bar to top mid if necessary to make room for all extras
    if (get_current_printer_config()->show_stats_on_progress_panel == SHOW_STATS_ON_PROGRESS_PANEL_ALL && CYD_SCREEN_HEIGHT_PX <= 320)
    {
        lv_obj_align(center_panel, LV_ALIGN_TOP_MID, 0, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX+(3 * CYD_SCREEN_GAP_PX));
    }
    else 
    {
        lv_obj_align(center_panel, LV_ALIGN_CENTER, 0, 0);
    }

    lv_obj_t * top_panel = lv_create_empty_panel(center_panel);
    lv_obj_set_size(top_panel, panel_width, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_top(top_panel, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(top_panel, 10, LV_PART_MAIN);
    lv_obj_clear_flag(top_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_layout_flex_row(top_panel);
    
    lv_obj_set_flex_align(top_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Filename
    lv_obj_t * label = lv_label_create(top_panel);
    lv_label_set_text(label, printer.print_filename);
    if (global_config.full_filenames) lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    else lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label, panel_width);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t * mid_panel = lv_create_empty_panel(center_panel);
    lv_obj_set_size(mid_panel, panel_width, LV_SIZE_CONTENT);
    lv_obj_clear_flag(mid_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_layout_flex_row(mid_panel);
    lv_obj_set_flex_align(mid_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * left_mid_panel = lv_create_empty_panel(mid_panel);
    lv_obj_set_size(left_mid_panel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(left_mid_panel, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * arc = lv_arc_create(left_mid_panel);
    lv_obj_align(arc, LV_ALIGN_LEFT_MID, 0, 0);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_range(arc, 0, 360);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);   /*Be sure the knob is not displayed*/
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(arc, progress_arc_update, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, arc, NULL);

    lv_obj_t * right_mid_panel = lv_create_empty_panel(mid_panel);
    // lv_obj_set_size(right_mid_panel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(right_mid_panel, 1);
    lv_obj_align(left_mid_panel, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_layout_flex_row(right_mid_panel);
    lv_obj_set_flex_align(right_mid_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

    lv_obj_t * text_panel = lv_create_empty_panel(right_mid_panel);
    lv_obj_align(text_panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(text_panel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_layout_flex_column(text_panel);

    lv_obj_t * time_panel = lv_create_empty_panel(right_mid_panel);
    lv_obj_align(time_panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(time_panel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_layout_flex_column(time_panel);

    label = lv_label_create(text_panel);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_text(label, "Total:");

    label = lv_label_create(text_panel);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label, "Rem:");

    label = lv_label_create(text_panel);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label, "Nozel:");

    label = lv_label_create(text_panel);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label, "Bed:");

    // Elapsed Time
    label = lv_label_create(time_panel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(label, update_printer_data_elapsed_time, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    // Remaining Time
    label = lv_label_create(time_panel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(label, update_printer_data_remaining_time, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    // Remaining Time
    label = lv_label_create(time_panel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(label, update_printer_hotend, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    // Remaining Time
    label = lv_label_create(time_panel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(label, update_printer_bed, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    lv_obj_t * arc_mid_panel = lv_create_empty_panel(arc);
    lv_obj_set_size(arc_mid_panel, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(arc_mid_panel, LV_ALIGN_CENTER, 0, 0);
    lv_layout_flex_column(arc_mid_panel);

    // Percentage
    label = lv_label_create(arc_mid_panel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(label, update_printer_data_percentage, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    label = lv_label_create(arc_mid_panel);
    lv_label_set_text(label, "???");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(label, update_printer_layer_printed, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);

    // Stop Button
    lv_obj_t * btn = lv_btn_create(panel);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -1 * CYD_SCREEN_GAP_PX, -1 * CYD_SCREEN_GAP_PX);
    lv_obj_set_size(btn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * button_size_mult, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * button_size_mult);
    lv_obj_add_event_cb(btn, btn_click_stop, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_STOP);
    lv_obj_center(label);

    // Resume Button
    if (printer.state == PRINTER_STATE_PAUSED){
        btn = lv_btn_create(panel);
        lv_obj_add_event_cb(btn, btn_click_resume, LV_EVENT_CLICKED, NULL);

        label = lv_label_create(btn);
        lv_label_set_text(label, LV_SYMBOL_PLAY);
        lv_obj_center(label);
    }
    // Pause Button
    else {
        btn = lv_btn_create(panel);
        lv_obj_add_event_cb(btn, btn_click_pause, LV_EVENT_CLICKED, NULL);

        label = lv_label_create(btn);
        lv_label_set_text(label, LV_SYMBOL_PAUSE);
        lv_obj_center(label);
    }

    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -2 * CYD_SCREEN_GAP_PX - CYD_SCREEN_MIN_BUTTON_WIDTH_PX * button_size_mult, -1 * CYD_SCREEN_GAP_PX);
    lv_obj_set_size(btn, CYD_SCREEN_MIN_BUTTON_WIDTH_PX * button_size_mult, CYD_SCREEN_MIN_BUTTON_HEIGHT_PX * button_size_mult);

    if (get_current_printer_config()->show_stats_on_progress_panel > SHOW_STATS_ON_PROGRESS_PANEL_NONE)
    {
        label = lv_label_create(panel);
        lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, CYD_SCREEN_GAP_PX, -1 * CYD_SCREEN_GAP_PX);
        lv_obj_set_style_text_font(label, &CYD_SCREEN_FONT_SMALL, 0);
        lv_obj_add_event_cb(label, update_printer_data_stats, LV_EVENT_MSG_RECEIVED, NULL);
        lv_msg_subsribe_obj(DATA_PRINTER_DATA, label, NULL);
    }
}