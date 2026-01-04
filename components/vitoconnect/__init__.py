import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_PROTOCOL, CONF_UPDATE_INTERVAL

CODEOWNERS = ["@dannerph"]

DEPENDENCIES = ["uart"]

MULTI_CONF = True

vitoconnect_ns = cg.esphome_ns.namespace("vitoconnect")
VitoConnect = vitoconnect_ns.class_("VitoConnect", uart.UARTDevice, cg.PollingComponent)

CONF_VITOCONNECT_ID = "vitoconnect_id"
CONF_QUEUE_SIZE = "queue_size"

OPTOLINK_PROTOCOL = {
    "P300": "P300",
    "KW": "KW",
}

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(VitoConnect),
        cv.Required(CONF_PROTOCOL): cv.enum(OPTOLINK_PROTOCOL, upper=True, space="_"),
        cv.Optional(
            CONF_UPDATE_INTERVAL, default="60s"
        ): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_QUEUE_SIZE, default=20): cv.int_range(min=1, max=100),
    }
).extend(uart.UART_DEVICE_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    cg.add(var.set_protocol(config[CONF_PROTOCOL]))
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))
    cg.add_build_flag(f"-DVITOWIFI_MAX_QUEUE_LENGTH={config[CONF_QUEUE_SIZE]}")
