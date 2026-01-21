import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ADDRESS, CONF_ID, CONF_OPTIONS
from . import VitoConnect, CONF_VITOCONNECT_ID, vitoconnect_ns

DEPENDENCIES = ["vitoconnect"]

VitoSelect = vitoconnect_ns.class_("VitoSelect", select.Select, cg.Component)

CONFIG_SCHEMA = select.select_schema(VitoSelect).extend(
    {
        cv.GenerateID(CONF_VITOCONNECT_ID): cv.use_id(VitoConnect),
        cv.Required(CONF_ADDRESS): cv.hex_int,
        cv.Optional("length", default=1): cv.positive_int,
        cv.Required(CONF_OPTIONS): cv.All(cv.ensure_list(cv.string), cv.Length(min=1)),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await select.register_select(var, config, options=config[CONF_OPTIONS])

    parent = await cg.get_variable(config[CONF_VITOCONNECT_ID])
    cg.add(var.set_parent(parent))
    cg.add(parent.register_select(var))
    
    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_length(config["length"]))
