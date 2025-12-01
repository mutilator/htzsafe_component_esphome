import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, binary_sensor
from esphome.const import CONF_ID, CONF_NAME

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["binary_sensor"]

htzsafe_ns = cg.esphome_ns.namespace("htzsafe")
HTZSafe = htzsafe_ns.class_("HTZSafe", cg.Component, uart.UARTDevice)
HTZSafeBinarySensor = htzsafe_ns.class_("HTZSafeBinarySensor", binary_sensor.BinarySensor, cg.Component)

CONF_DEVICES = "devices"
CONF_IDENTIFIER = "identifier"

DEVICE_SCHEMA = binary_sensor.binary_sensor_schema(
    HTZSafeBinarySensor,
    device_class="occupancy"
).extend({
    cv.Required(CONF_IDENTIFIER): cv.hex_int,
})

CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(HTZSafe),
        cv.Required(CONF_DEVICES): cv.ensure_list(DEVICE_SCHEMA),
    }).extend(uart.UART_DEVICE_SCHEMA),
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    for device_config in config[CONF_DEVICES]:
        identifier = device_config[CONF_IDENTIFIER]
        sens = await binary_sensor.new_binary_sensor(device_config)
        cg.add(sens.set_parent(var))
        cg.add(sens.set_device_id(identifier))
        cg.add(var.register_sensor(sens, identifier))
