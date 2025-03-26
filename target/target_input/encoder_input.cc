#include "encoder_input.hh"

#include <array>

using namespace hal;

constexpr pcnt_unit_config_t kPcntUnitConfig = {
    .low_limit = -4,
    .high_limit = 4,
    .flags {.accum_count = 1},
};
constexpr pcnt_glitch_filter_config_t kFilterConfig = {
    .max_glitch_ns = 1000,
};

namespace
{

class DummyListener : public IInput::IListener
{
public:
    void OnInput(const IInput::Event& event) final
    {
    }
};

DummyListener g_dummy_listener;

} // namespace


EncoderInput::EncoderInput(uint8_t pin_a, uint8_t pin_b, uint8_t pin_button, uint8_t switch_up_pin)
    : m_pin_button(static_cast<gpio_num_t>(pin_button))
    , m_pin_switch_up(static_cast<gpio_num_t>(switch_up_pin))
    , m_listener(&g_dummy_listener)
    , m_button_timestamp(os::GetTimeStamp())
{
    ESP_ERROR_CHECK(pcnt_new_unit(&kPcntUnitConfig, &m_pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(m_pcnt_unit, &kFilterConfig));

    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = pin_a,
        .level_gpio_num = pin_b,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(m_pcnt_unit, &chan_a_config, &pcnt_chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = pin_b,
        .level_gpio_num = pin_a,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(m_pcnt_unit, &chan_b_config, &pcnt_chan_b));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    constexpr auto kWatchPoints = std::array {-4, 4};
    for (size_t i = 0; i < kWatchPoints.size(); i++)
    {
        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(m_pcnt_unit, kWatchPoints[i]));
    }
    pcnt_event_callbacks_t cbs = {
        .on_reach = EncoderInput::StaticPcntOnReach,
    };
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(m_pcnt_unit, &cbs, this));

    ESP_ERROR_CHECK(pcnt_unit_enable(m_pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(m_pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(m_pcnt_unit));

    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1 << m_pin_button;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    //configure GPIO with the given settings
    gpio_config(&io_conf);
    gpio_install_isr_service(0);

    gpio_isr_handler_add(m_pin_button, EncoderInput::StaticButtonIsr, static_cast<void*>(this));
}

void
EncoderInput::AttachListener(IListener* listener)
{
    m_listener = listener;
}

IInput::State
EncoderInput::GetState()
{
    auto switch_up_active = gpio_get_level(m_pin_switch_up);
    auto button = gpio_get_level(m_pin_button);

    uint8_t state = 0;

    if (switch_up_active)
    {
        state |= std::to_underlying(IInput::StateType::kSwitchUp);
    }
    if (button)
    {
        state |= std::to_underlying(IInput::StateType::kButtonDown);
    }

    return IInput::State(state);
}

void
EncoderInput::PcntOnReach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t* edata)
{
    IInput::Event event = {
        .button = 0,
        .type = IInput::EventType::kButtonDown,
    };

    if (edata->watch_point_value < 0)
    {
        event.type = IInput::EventType::kLeft;
    }
    else if (edata->watch_point_value > 0)
    {
        event.type = IInput::EventType::kRight;
    }

    m_listener->OnInput(event);
}

bool
EncoderInput::StaticPcntOnReach(pcnt_unit_handle_t unit,
                                const pcnt_watch_event_data_t* edata,
                                void* user_ctx)
{
    auto p_this = static_cast<EncoderInput*>(user_ctx);
    p_this->PcntOnReach(unit, edata);

    return false;
}

void
EncoderInput::ButtonIsr()
{
    auto now = os::GetTimeStamp();

    if (now - m_button_timestamp < 50ms)
    {
        return;
    }
    m_button_timestamp = now;

    IInput::Event event = {
        .button = 0,
        .type = gpio_get_level(m_pin_button) ? IInput::EventType::kButtonDown
                                             : IInput::EventType::kButtonUp,
    };

    m_listener->OnInput(event);
}

void
EncoderInput::StaticButtonIsr(void* arg)
{
    auto p_this = static_cast<EncoderInput*>(arg);
    p_this->ButtonIsr();
}
