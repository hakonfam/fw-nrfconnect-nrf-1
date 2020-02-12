static struct device *flash_dev;
static const struct flash_pages_layout *layout;

static u8_t *write_buf;
static size_t write_buf_pos;

static int offset;
static size_t available_size;
static int start_address;

static dfu_target_callback_t callback;

static int write_buf_to_flash(size_t len)
{
	int rc;
	int write_address = start_address + offset;

	if (flash_dev == NULL) {
		return -EFAULT;
	}

	flash_write_protection_set(flash_dev, false);
	rc = flash_erase(flash_dev, write_address, layout->pages_size);
	if (rc != 0) {
		LOG_ERR("flash erase failed: %d", rc);
		goto out;
	}

	rc = flash_write(flash_dev, write_address, write_buf,
			 len);
	if (rc != 0) {
		LOG_ERR("flash write failed: %d", rc);
		goto out;
	}

	if (callback) {
		rc = flash_read(flash_dev, write_address, write_buf,
				len);
		if (rc != 0) {
			LOG_ERR("flash read failed: %d", rc);
			goto out;
		}

		callback(write_buf, len, offset);
	}

out:
	flash_write_protection_set(flash_dev, true);
	return rc;
}

int flash_img_raw_cfg(const char *dev_name, size_t start, size_t end,
		      void *buf, size_t buf_len, size_t *available)
{
	size_t total_size;
	size_t layout_size;

	flash_dev = device_get_binding(dev_name);
	if (!flash_dev) {
		LOG_ERR("Unable to initialize with configured device");
		return -EFAULT;
	}

	const struct flash_driver_api *api = flash_dev->driver_api;

	api->page_layout(flash_dev, &layout, &layout_size);

	/* Only single sized page layout supported */
	__ASSERT_NO_MSG(layout_size == 1);

	start_address = start;
	total_size = layout->pages_count * layout->pages_size;

	if (end > total_size ||
	    (end != 0 && start > end) ||
	    start_address % api->write_block_size ||
	    buf == NULL ||
	    buf_len < layout->pages_size) {
		LOG_ERR("Incorrect parameter");
		return -EFAULT;
	}

	*available  = (end == 0 ? total_size : end) - start_address;

	write_buf = buf;

	return 0;
}

int flash_img_raw_set_callback(dfu_target_callback_t callback_in)
{
	if (!callback_in) {
		return -EFAULT;
	}

	callback = callback_in;

	return 0;
}

int flash_img_raw_offset_get(size_t *out)
{
	*out = offset + write_buf_pos;

	return 0;
}

int flash_img_raw_write(const void *const buf, size_t len)
{
	int rc;
	const u8_t *bufp = (const u8_t *)buf;

	while (write_buf_pos + len >= layout->pages_size) {
		size_t bytes_to_write = layout->pages_size - write_buf_pos;

		memcpy(write_buf+write_buf_pos, bufp, bytes_to_write);
		rc = write_buf_to_flash(layout->pages_size);
		if (rc) {
			return rc;
		}

		/* Only update offset in per page increments */
		offset += layout->pages_size;

		/* In case there are more bytes, store them in buffer */
		bufp += bytes_to_write;
		len -= bytes_to_write;
		write_buf_pos = 0;
	}

	if (len) {
		__ASSERT_NO_MSG(write_buf_pos + len < layout->pages_size);
		memcpy(write_buf+write_buf_pos, bufp, len);
		write_buf_pos += len;
	}

	return 0;
}

int flash_img_raw_done(void)
{
	int rc = 0;

	rc = write_buf_to_flash(write_buf_pos);
	if (rc != 0) {
		LOG_ERR("write_buf_to_flash error: %d", rc);
		return rc;
	}

	offset = 0;
	write_buf_pos = 0;

	/* We leave flash_dev intact, as we might want to pick up the
	 * write operation where it left off at a later point.
	 */

	return rc;
}
