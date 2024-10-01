/*
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/w1_sensor.h>
#include <zephyr/drivers/w1.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

struct device const *devices[5];
int count = 0;

void w1_search_callback(struct w1_rom rom, void *user_data)
{
	LOG_INF("Device found; family: 0x%02x, serial: 0x%016llx", rom.family,
		w1_rom_to_uint64(&rom));
	struct sensor_value *romval;

	char sensor_label[255];

	sprintf(sensor_label, "SENSOR_DS18B20_%d", count);

	devices[count] = device_get_binding(sensor_label);
	struct device const *dev = devices[count++];
	if (dev == NULL) {
		/* No such node, or the node does not have status "okay". */
		printk("\nError: no device found.\n");
		return;
	}

   if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
				   "check the driver initialization logs for errors.\n",
				   dev->name);
		return;
	}

	romval = malloc(sizeof (struct sensor_value));
	w1_rom_to_sensor_value(&rom, romval);

	 sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_W1_ROM, romval);
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(w1));
	int res;
	if (!device_is_ready(dev)) {
		LOG_ERR("Device not ready");
		return 0;
	}

	int num_devices = w1_search_rom(dev, w1_search_callback, NULL);

	LOG_INF("Number of devices found on bus: %d", num_devices);

	while (true) {
		int i;
		for (i = 0 ; i < num_devices; i++) {
			struct sensor_value temp;

			res = sensor_sample_fetch(devices[i]);
			if (res != 0) {
				printk("[%d] sample_fetch() failed: %d\n",i, res);
//				return res;
			}

			res = sensor_channel_get(devices[i], SENSOR_CHAN_AMBIENT_TEMP, &temp);
			if (res != 0) {
				printk("[%d] channel_get() failed: %d\n",i, res);
	//			return res;
			}

			printk("[%d] Temp: %d.%06d\n", i, temp.val1, temp.val2);
			k_sleep(K_MSEC(2000));
		}
	}

	return 0;
}
