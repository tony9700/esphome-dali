#pragma once

#include <esphome.h>
#include <vector>
#include "dali.h"

namespace esphome {
namespace dali {

enum class DaliInitMode {
    DiscoverOnly,
    InitializeUnassigned,
    InitializeAll
};

class DaliBusComponent : public Component, public DaliPort {
public:
    DaliBusComponent()
        : Component { }
        , dali { *this }
    { }

    void setup() override;
    void loop() override;
    void dump_config() override;

    void set_tx_pin(GPIOPin* tx_pin) { m_txPin = tx_pin; }
    void set_rx_pin(GPIOPin* rx_pin) { m_rxPin = rx_pin; }

    /// @brief Perform automatic device discovery on setup.
    /// Light components will automatically be created and appear in HomeAssistant
    void do_device_discovery() { m_discovery = true; }

    /// @brief Initialize long and short addresses for devices on the bus.
    /// @param mode 
    //          InitializeUnassigned - only devices that do not yet have an assigned short address
    ///         InitializeAll - all devices on the bus
    /// @note
    void do_initialize_addresses(DaliInitMode mode = DaliInitMode::InitializeUnassigned) { m_initialize_addresses = mode; }

    // NOTE: Must have a higher priority number than the components that depend on this.
    // ie, this must be initialized first.
    float get_setup_priority() const override { return setup_priority::HARDWARE; }

    void register_static_addr(short_addr_t short_addr) {
        if (short_addr < ADDR_SHORT_MAX) {
            m_addresses[short_addr] = 0xFFFFFF;
        }
    }

    DaliMaster dali;
    void queue_primary_n(uint8_t address, bool on, const uint16_t *levels);
    bool is_busy() const { return active_ || !pending_.empty(); }
    uint32_t get_queue_length() const { return active_ ? frames_.size() - frame_index_ : 0; }
    uint32_t get_pending_updates() const { return pending_.size(); }
    uint32_t get_updates_completed() const { return completed_; }
    uint32_t get_updates_superseded() const { return superseded_; }
    uint32_t get_average_update_time() const { return completed_ ? update_sum_ / completed_ : 0; }

    uint32_t get_tx_count() const { return tx_count_; }
    uint32_t get_rx_count() const { return rx_count_; }
    uint32_t get_rx_timeout_count() const { return rx_timeout_count_; }
    uint32_t get_update_time() const { return update_time_; }
    uint32_t get_max_update_time() const { return max_update_time_; }
    void record_update_time(uint32_t elapsed) {
        update_time_ = elapsed;
        if (elapsed > max_update_time_) max_update_time_ = elapsed;
    }

public: // DaliPort
    void resetBus() override;
    void sendForwardFrame(uint8_t address, uint8_t data) override;
    uint8_t receiveBackwardFrame(unsigned long timeout_ms = 100) override;

private:
    struct PrimaryRequest {
        uint8_t address;
        bool on;
        uint16_t levels[4];
        uint32_t requested;
    };
    struct Frame { uint8_t address; uint8_t data; };
    std::vector<PrimaryRequest> pending_;
    std::vector<Frame> frames_;
    PrimaryRequest current_{};
    PrimaryRequest last_{};
    bool last_valid_{false};
    bool active_{false};
    bool queue_sending_{false};
    size_t frame_index_{0};
    uint32_t next_frame_at_{0};
    uint32_t completed_{0};
    uint32_t superseded_{0};
    uint64_t update_sum_{0};
    void start_primary_();
    void service_primary_();
    void finish_active_();
    uint32_t tx_count_{0};
    uint32_t rx_count_{0};
    uint32_t rx_timeout_count_{0};
    uint32_t update_time_{0};
    uint32_t max_update_time_{0};
    void writeBit(bool bit);
    void writeByte(uint8_t b);
    uint8_t readByte();

    void create_light_component(short_addr_t short_addr, uint32_t long_addr);

    GPIOPin* m_rxPin;
    GPIOPin* m_txPin;

    bool m_discovery = false;
    DaliInitMode m_initialize_addresses = DaliInitMode::DiscoverOnly;
    uint32_t m_addresses[ADDR_SHORT_MAX+1] = {0};

    // Dynamic lights created during discovery are not in ESPHome's looping_components_
    // (that list is fixed at compile time). We drive their loop() manually.
    std::vector<light::LightState*> m_dynamic_lights;
};

}  // namespace dali
}  // namespace esphome
