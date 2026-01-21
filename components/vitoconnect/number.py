import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    CONF_ADDRESS,
    CONF_ID,
    CONF_MIN_VALUE,
    CONF_MAX_VALUE,
    CONF_STEP,
)
from . import VitoConnect, CONF_VITOCONNECT_ID, vitoconnect_ns

DEPENDENCIES = ["vitoconnect"]

VitoNumber = vitoconnect_ns.class_("VitoNumber", number.Number, cg.Component)

CONFIG_SCHEMA = number.number_schema(VitoNumber).extend(
    {
        cv.GenerateID(CONF_VITOCONNECT_ID): cv.use_id(VitoConnect),
        cv.Required(CONF_ADDRESS): cv.hex_int,
        cv.Optional("length", default=1): cv.positive_int,
        cv.Optional("restore_value", default=False): cv.boolean,
        cv.Optional(CONF_MIN_VALUE): cv.float_,
        cv.Optional(CONF_MAX_VALUE): cv.float_,
        cv.Optional(CONF_STEP): cv.positive_float,
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await number.register_number(
        var,
        config,
        min_value=config[CONF_MIN_VALUE],
        max_value=config[CONF_MAX_VALUE],
        step=config[CONF_STEP],
    )

    parent = await cg.get_variable(config[CONF_VITOCONNECT_ID])
    cg.add(var.set_parent(parent))
    cg.add(parent.register_number(var))
    
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_length(config["length"]))
