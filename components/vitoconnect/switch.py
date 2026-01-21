import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID, CONF_ADDRESS
from . import vitoconnect_ns, VitoConnect, CONF_VITOCONNECT_ID

DEPENDENCIES = ["vitoconnect"]

VitoSwitch = vitoconnect_ns.class_("VitoSwitch", switch.Switch, cg.Component)

CONFIG_SCHEMA = switch.switch_schema(VitoSwitch).extend(
    {
        cv.GenerateID(): cv.declare_id(VitoSwitch),
        cv.GenerateID(CONF_VITOCONNECT_ID): cv.use_id(VitoConnect),
        cv.Required(CONF_ADDRESS): cv.hex_int,
        # Default length for switch is usually 1 byte
        cv.Optional("length", default=1): cv.positive_int, 
        cv.Optional("on_value", default=[0x01]): cv.ensure_list(cv.hex_uint8_t),
        cv.Optional("off_value", default=[0x00]): cv.ensure_list(cv.hex_uint8_t),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await switch.register_switch(var, config)

    parent = await cg.get_variable(config[CONF_VITOCONNECT_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_length(config["length"]))
    
    # helper to create vector from list
    on_payload = config["on_value"]
    off_payload = config["off_value"]
    
    cg.add(var.set_on_value(on_payload))
    cg.add(var.set_off_value(off_payload))

    cg.add(parent.register_switch(var))
