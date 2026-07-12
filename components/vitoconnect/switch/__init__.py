import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ADDRESS

from .. import CONF_CHECK_ONCE, CONF_VITOCONNECT_ID, VitoConnect, vitoconnect_ns

DEPENDENCIES = ["vitoconnect"]
OPTOLINKSwitch = vitoconnect_ns.class_("OPTOLINKSwitch", switch.Switch)

CONFIG_SCHEMA = switch.switch_schema(OPTOLINKSwitch).extend(
    {
        cv.GenerateID(): cv.declare_id(OPTOLINKSwitch),
        cv.GenerateID(CONF_VITOCONNECT_ID): cv.use_id(VitoConnect),
        cv.Required(CONF_ADDRESS): cv.uint16_t,
        cv.Optional(CONF_CHECK_ONCE, default=False): cv.boolean,
    }
)


async def to_code(config):
    var = await switch.new_switch(config)

    cg.add(var.setAddress(config[CONF_ADDRESS]))
    cg.add(var.setLength(1))
    cg.add(var.setCheckOnce(config[CONF_CHECK_ONCE]))

    hub = await cg.get_variable(config[CONF_VITOCONNECT_ID])
    cg.add(var.set_parent(hub))
    cg.add(hub.register_datapoint(var))
