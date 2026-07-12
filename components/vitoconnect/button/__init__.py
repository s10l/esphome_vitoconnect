import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button

from .. import CONF_VITOCONNECT_ID, VitoConnect, vitoconnect_ns

DEPENDENCIES = ["vitoconnect"]
OPTOLINKRefreshOnceButton = vitoconnect_ns.class_(
    "OPTOLINKRefreshOnceButton", button.Button
)

CONFIG_SCHEMA = button.button_schema(OPTOLINKRefreshOnceButton).extend(
    {
        cv.GenerateID(): cv.declare_id(OPTOLINKRefreshOnceButton),
        cv.GenerateID(CONF_VITOCONNECT_ID): cv.use_id(VitoConnect),
    }
)


async def to_code(config):
    var = await button.new_button(config)
    hub = await cg.get_variable(config[CONF_VITOCONNECT_ID])
    cg.add(var.set_parent(hub))
