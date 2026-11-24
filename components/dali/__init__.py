from typing import OrderedDict
from esphome import pins
from esphome.const import CONF_ID, CONF_RX_PIN, CONF_TX_PIN, CONF_DISCOVERY
from esphome.core import CORE

import esphome.codegen as cg
import esphome.config_validation as cv

AUTO_LOAD = ["light", "output"]

CONF_DRIVER = 'driver'
CONF_RMT_INVERT_TX = 'rmt_invert_tx'
CONF_RMT_INVERT_RX = 'rmt_invert_rx'

CONF_DALI_BUS = 'dali_bus'
CONF_INITIALIZE_ADDRESSES = 'initialize_addresses'

dali_ns = cg.esphome_ns.namespace('dali')
dali_lib_ns = cg.global_ns
DaliBusComponent = dali_ns.class_('DaliBusComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.Optional(CONF_DRIVER, default='bitbang'): cv.one_of('bitbang', 'rmt', lower=True),
    cv.Optional(CONF_RMT_INVERT_TX, default=True): cv.boolean,
    cv.Optional(CONF_RMT_INVERT_RX, default=True): cv.boolean,
    cv.GenerateID(): cv.declare_id(DaliBusComponent),
    cv.Required(CONF_RX_PIN): pins.gpio_input_pin_schema,
    cv.Required(CONF_TX_PIN): pins.gpio_output_pin_schema,
    cv.Optional(CONF_DISCOVERY): cv.All(cv.requires_component("light"), cv.boolean),
    cv.Optional(CONF_INITIALIZE_ADDRESSES): cv.boolean,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config: OrderedDict):
    var = cg.new_Pvariable(config[CONF_ID])
    bus = await cg.register_component(var, config)

    rx_pin = await cg.gpio_pin_expression(config[CONF_RX_PIN])
    cg.add(var.set_rx_pin(rx_pin))
    
    tx_pin = await cg.gpio_pin_expression(config[CONF_TX_PIN])
    cg.add(var.set_tx_pin(tx_pin))

    if config[CONF_DRIVER] == 'rmt':
        if not CORE.is_esp32 or CORE.target_framework != 'esp-idf':
            raise cv.Invalid('DALI RMT requires ESP-IDF')
        from esphome.components import esp32
        esp32.add_idf_component(name='espressif/dali', ref='1.1.0')
        cg.add_define('USE_GEAS_DALI_RMT')
        cg.add(var.set_rmt_config(config[CONF_TX_PIN]['number'], config[CONF_RX_PIN]['number'],
                                  config[CONF_RMT_INVERT_TX], config[CONF_RMT_INVERT_RX]))

    if config.get(CONF_DISCOVERY, False):
        cg.add(var.do_device_discovery())

        # When discovery is enabled but no light components are defined
        # in the YAML, we need to make it look like we have a light 
        # defined so it will compile in support. Without this, USE_LIGHT
        # will not be defined.
        #
        # This can be done by registering this bus component as a light,
        # making the core think there is at least one light defined.
        CORE.register_platform_component("light", bus)

    if config.get(CONF_INITIALIZE_ADDRESSES, False):
        cg.add(var.do_initialize_addresses())
