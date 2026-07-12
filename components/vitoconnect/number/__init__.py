import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import CONF_ADDRESS, CONF_LENGTH, CONF_MAX_VALUE, CONF_MIN_VALUE, CONF_STEP

from .. import CONF_CHECK_ONCE, CONF_VITOCONNECT_ID, VitoConnect, vitoconnect_ns

DEPENDENCIES = ["vitoconnect"]
OPTOLINKNumber = vitoconnect_ns.class_("OPTOLINKNumber", number.Number)

CONFIG_SCHEMA = number.number_schema(OPTOLINKNumber).extend(
    {
        cv.GenerateID(): cv.declare_id(OPTOLINKNumber),
        cv.GenerateID(CONF_VITOCONNECT_ID): cv.use_id(VitoConnect),
        cv.Required(CONF_ADDRESS): cv.uint16_t,
        cv.Required(CONF_LENGTH): cv.uint8_t,
        cv.Required(CONF_MIN_VALUE): cv.float_,
        cv.Required(CONF_MAX_VALUE): cv.float_,
        cv.Optional(CONF_STEP, default=1): cv.float_,
        cv.Optional(CONF_CHECK_ONCE, default=False): cv.boolean,
    }
)


async def to_code(config):
    var = await number.new_number(
        config,
        min_value=config[CONF_MIN_VALUE],
        max_value=config[CONF_MAX_VALUE],
        step=config[CONF_STEP],
    )

    cg.add(var.setAddress(config[CONF_ADDRESS]))
    cg.add(var.setLength(config[CONF_LENGTH]))
    cg.add(var.setCheckOnce(config[CONF_CHECK_ONCE]))

    hub = await cg.get_variable(config[CONF_VITOCONNECT_ID])
    cg.add(var.set_parent(hub))
    cg.add(hub.register_datapoint(var))
