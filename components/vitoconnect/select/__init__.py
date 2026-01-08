import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select
from esphome.const import CONF_ADDRESS, CONF_LENGTH, CONF_OPTIONS
from .. import vitoconnect_ns, VitoConnect, CONF_VITOCONNECT_ID

DEPENDENCIES = ["vitoconnect"]
CODEOWNERS = ["@s10l"]

OPTOLINKSelect = vitoconnect_ns.class_("OPTOLINKSelect", select.Select)

# options in YAML are expected as a list of mappings with 'value' and 'label',
# e.g.:
# options:
#   - value: 0
#     label: "Standby"
#   - value: 1
#     label: "DHW Only"
#   - value: 2
#     label: "Heating and DHW"
OPTIONS_SCHEMA = cv.Schema({
    cv.Required("value"): cv.int_,
    cv.Required("label"): cv.string,
})

CONFIG_SCHEMA = select.select_schema(OPTOLINKSelect).extend({
    cv.GenerateID(): cv.declare_id(OPTOLINKSelect),
    cv.GenerateID(CONF_VITOCONNECT_ID): cv.use_id(VitoConnect),
    cv.Required(CONF_ADDRESS): cv.uint16_t,
    cv.Required(CONF_LENGTH): cv.uint8_t,
    cv.Required(CONF_OPTIONS): cv.ensure_list(OPTIONS_SCHEMA),
})


async def to_code(config):
    # Extract labels from options if provided, otherwise use empty list
    if CONF_OPTIONS in config:
        labels = [o["label"] for o in config[CONF_OPTIONS]]
        values = [o["value"] for o in config[CONF_OPTIONS]]
    else:
        labels = []
        values = []
    
    # Create select with required options parameter
    var = await select.new_select(config, options=labels)

    # Add configuration to datapoint
    cg.add(var.setAddress(config[CONF_ADDRESS]))
    cg.add(var.setLength(config[CONF_LENGTH]))

    # Pass labels and numeric values to the C++ class for mapping
    cg.add(var.set_option_labels(labels))
    cg.add(var.set_option_values(values))

    # Add sensor to component hub (VitoConnect)
    hub = await cg.get_variable(config[CONF_VITOCONNECT_ID])
    cg.add(hub.register_datapoint(var))
