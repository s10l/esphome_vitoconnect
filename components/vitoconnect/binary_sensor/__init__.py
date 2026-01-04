import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ADDRESS

from .. import CONF_VITOCONNECT_ID, CONF_PRIORITY, VitoConnect, vitoconnect_ns

DEPENDENCIES = ["vitoconnect"]
OPTOLINKBinarySensor = vitoconnect_ns.class_(
    "OPTOLINKBinarySensor", binary_sensor.BinarySensor
)

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(OPTOLINKBinarySensor).extend(
    {
        cv.GenerateID(): cv.declare_id(OPTOLINKBinarySensor),
        cv.GenerateID(CONF_VITOCONNECT_ID): cv.use_id(VitoConnect),
        cv.Required(CONF_ADDRESS): cv.uint16_t,
        cv.Optional(CONF_PRIORITY, default=2): cv.int_range(min=1, max=3),
    }
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)

    # Add configuration to datapoint
    cg.add(var.setAddress(config[CONF_ADDRESS]))
    cg.add(var.setLength(1))
    cg.add(var.setPriority(config[CONF_PRIORITY]))

    # Add sensor to component hub (VitoConnect)
    hub = await cg.get_variable(config[CONF_VITOCONNECT_ID])
    cg.add(hub.register_datapoint(var))
