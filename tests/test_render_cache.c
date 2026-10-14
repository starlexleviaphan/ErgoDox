// tests/test_render_cache.c
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define NUM_INSPECTED_LAYERS 15

/* Render State Cache simulation */
static int rendered_layer = -1;
static int rendered_is_left = -1;
static int rendered_dashboard_mode = -1;

/* Inspector State */
static uint8_t current_layer = 0;
static bool current_is_left_half = true;
static bool inspector_active = false;
static bool is_dashboard_mode = false;

/* Metrics */
static int render_count = 0;
static int scheduled_delay_ms = -1;
static int work_schedule_count = 0;

static void mock_work_reschedule(int delay_ms) {
    scheduled_delay_ms = delay_ms;
    work_schedule_count++;
}

static void mock_render(void) {
    if (!inspector_active) {
        return;
    }

    if (current_layer >= NUM_INSPECTED_LAYERS) {
        current_layer = 0;
    }

    /* Early exit if render state has not changed (0 ms no-op) */
    if (rendered_layer == (int)current_layer &&
        rendered_is_left == (int)current_is_left_half &&
        rendered_dashboard_mode == (int)is_dashboard_mode) {
        return;
    }

    /* Perform redraw */
    render_count++;

    /* Update cache */
    rendered_layer = (int)current_layer;
    rendered_is_left = (int)current_is_left_half;
    rendered_dashboard_mode = (int)is_dashboard_mode;
}

static void mock_show(uint8_t layer, bool is_left_half) {
    if (layer >= NUM_INSPECTED_LAYERS) {
        layer = 0;
    }
    current_layer = layer;
    current_is_left_half = is_left_half;
    is_dashboard_mode = false;
    inspector_active = true;
    mock_work_reschedule(0);
}

static void mock_hide(void) {
    inspector_active = false;
    is_dashboard_mode = false;
    rendered_layer = -1;
    rendered_is_left = -1;
    rendered_dashboard_mode = -1;
    mock_work_reschedule(0);
}

static void mock_on_key_position(bool is_right) {
    bool new_is_left = !is_right;
    if (new_is_left != current_is_left_half) {
        current_is_left_half = new_is_left;
        if (inspector_active) {
            mock_work_reschedule(50);
        }
    }
}

static void mock_on_layer_change(uint8_t layer) {
    if (layer >= NUM_INSPECTED_LAYERS) {
        layer = 0;
    }
    if (layer != current_layer) {
        current_layer = layer;
        if (inspector_active) {
            mock_work_reschedule(50);
        }
    }
}

int main(void) {
    printf("Starting render cache and debouncing simulation tests...\n");

    /* 1. Initial Show */
    mock_show(0, true);
    assert(work_schedule_count == 1);
    assert(scheduled_delay_ms == 0); /* Immediate */

    /* Run the scheduled render */
    mock_render();
    assert(render_count == 1);
    assert(rendered_layer == 0);
    assert(rendered_is_left == 1);
    assert(rendered_dashboard_mode == 0);

    /* 2. Redundant render call (e.g. from display refresh / touchpad swipe on same state) */
    mock_render();
    assert(render_count == 1); /* Must NOT re-render (cache hit, 0 ms) */

    /* 3. Rapid layer changes debounced to 50ms */
    work_schedule_count = 0;
    mock_on_layer_change(1);
    assert(scheduled_delay_ms == 50);
    mock_on_layer_change(2);
    assert(scheduled_delay_ms == 50);
    mock_on_layer_change(3);
    assert(scheduled_delay_ms == 50);
    assert(work_schedule_count == 3);
    assert(render_count == 1); /* No premature renders during burst */

    /* Simulating debounce timer expiry: single render execution */
    mock_render();
    assert(render_count == 2);
    assert(rendered_layer == 3);

    /* 4. Switching hands (key position change) */
    mock_on_key_position(true); /* Right half */
    assert(scheduled_delay_ms == 50);
    mock_render();
    assert(render_count == 3);
    assert(rendered_is_left == 0);

    /* Key press on the SAME half: should NOT schedule or render */
    work_schedule_count = 0;
    mock_on_key_position(true); /* Right half again */
    assert(work_schedule_count == 0); /* Ignored */
    mock_render();
    assert(render_count == 3); /* Cache hit */

    /* 5. Hide resets cache */
    mock_hide();
    assert(!inspector_active);
    assert(rendered_layer == -1);
    assert(rendered_is_left == -1);
    assert(rendered_dashboard_mode == -1);

    /* Render while inactive: no-op */
    mock_render();
    assert(render_count == 3);

    /* 6. Re-entering triggers fresh draw */
    mock_show(3, false);
    mock_render();
    assert(render_count == 4); /* Cache was reset, so fresh draw occurred */
    assert(rendered_layer == 3);
    assert(rendered_is_left == 0);

    printf("PASS: All render cache and debouncing simulation assertions succeeded.\n");
    return 0;
}
