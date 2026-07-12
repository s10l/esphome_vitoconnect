import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ADDRESS, CONF_LENGTH

from .. import (
    CONF_CHECK_ONCE,
    CONF_VITOCONNECT_ID,
    VitoConnect,
    vitoconnect_ns,
)

CONF_UNKNOWN_VALUE = "unknown_value"
CONF_VALUES = "values"

DEPENDENCIES = ["vitoconnect"]
OPTOLINKTextSensor = vitoconnect_ns.class_(
    "OPTOLINKTextSensor", text_sensor.TextSensor
)

VALUE_MAP_SCHEMA = cv.All(
    cv.Schema({cv.int_range(min=0, max=65535): cv.string}),
    cv.Length(min=1),
)

CONFIG_SCHEMA = text_sensor.text_sensor_schema(OPTOLINKTextSensor).extend(
    {
        cv.GenerateID(): cv.declare_id(OPTOLINKTextSensor),
        cv.GenerateID(CONF_VITOCONNECT_ID): cv.use_id(VitoConnect),
        cv.Required(CONF_ADDRESS): cv.uint16_t,
        cv.Required(CONF_LENGTH): cv.int_range(min=1, max=2),
        cv.Required(CONF_VALUES): VALUE_MAP_SCHEMA,
        cv.Optional(CONF_UNKNOWN_VALUE): cv.string,
        cv.Optional(CONF_CHECK_ONCE, default=False): cv.boolean,
    }
)


async def to_code(config):
    var = await text_sensor.new_text_sensor(config)

    cg.add(var.setAddress(config[CONF_ADDRESS]))
    cg.add(var.setLength(config[CONF_LENGTH]))
    cg.add(var.setCheckOnce(config[CONF_CHECK_ONCE]))
    if CONF_UNKNOWN_VALUE in config:
        cg.add(var.set_unknown_value(config[CONF_UNKNOWN_VALUE]))

    for value, label in config[CONF_VALUES].items():
        cg.add(var.add_mapping(value, label))

    hub = await cg.get_variable(config[CONF_VITOCONNECT_ID])
    cg.add(hub.register_datapoint(var))
