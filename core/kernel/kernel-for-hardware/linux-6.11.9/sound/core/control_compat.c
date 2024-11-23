// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * compat ioctls for control API
 *
 *   Copyright (c) by Takashi Iwai <tiwai@suse.de>
 */

/* this file included from control.c */

#include <linux/compat.h>
#include <linux/slab.h>

struct snd_ctl_elem_list32 {
	u32 offset;
	u32 space;
	u32 used;
	u32 count;
	u32 pids;
	unsigned char reserved[50];
} /* don't set packed attribute here */;

static int snd_ctl_elem_list_compat(struct snd_card *card,
				    struct snd_ctl_elem_list32 __user *data32)
{
	struct snd_ctl_elem_list data = {};
	compat_caddr_t ptr;
	int err;

	/* offset, space, used, count */
	if (copy_from_user(&data, data32, 4 * sizeof(u32)))
		return -EFAULT;
	/* pids */
	if (get_user(ptr, &data32->pids))
		return -EFAULT;
	data.pids = compat_ptr(ptr);
	err = snd_ctl_elem_list(card, &data);
	if (err < 0)
		return err;
	/* copy the result */
	if (copy_to_user(data32, &data, 4 * sizeof(u32)))
		return -EFAULT;
	return 0;
}

/*
 * control element info
 * it uses union, so the things are not easy..
 */

struct snd_ctl_elem_info32 {
	struct snd_ctl_elem_id id; // the size of struct is same
	s32 type;
	u32 access;
	u32 count;
	s32 owner;
	union {
		struct {
			s32 min;
			s32 max;
			s32 step;
		} integer;
		struct {
			u64 min;
			u64 max;
			u64 step;
		} integer64;
		struct {
			u32 items;
			u32 item;
			char name[64];
			u64 names_ptr;
			u32 names_length;
		} enumerated;
		unsigned char reserved[128];
	} value;
	unsigned char reserved[64];
} __packed;

static int snd_ctl_elem_info_compat(struct snd_ctl_file *ctl,
				    struct snd_ctl_elem_info32 __user *data32)
{
	struct snd_card *card = ctl->card;
	struct snd_ctl_elem_info *data __free(kfree) = NULL;
	int err;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (! data)
		return -ENOMEM;

	/* copy id */
	if (copy_from_user(&data->id, &data32->id, sizeof(data->id)))
		return -EFAULT;
	/* we need to copy the item index.
	 * hope this doesn't break anything..
	 */
	if (get_user(data->value.enumerated.item, &data32->value.enumerated.item))
		return -EFAULT;

	err = snd_power_ref_and_wait(card);
	if (err < 0)
		return err;
	err = snd_ctl_elem_info(ctl, data);
	snd_power_unref(card);
	if (err < 0)
		return err;
	/* restore info to 32bit */
	/* id, type, access, count */
	if (copy_to_user(&data32->id, &data->id, sizeof(data->id)) ||
	    copy_to_user(&data32->type, &data->type, 3 * sizeof(u32)))
		return -EFAULT;
	if (put_user(data->owner, &data32->owner))
		return -EFAULT;
	switch (data->type) {
	case SNDRV_CTL_ELEM_TYPE_BOOLEAN:
	case SNDRV_CTL_ELEM_TYPE_INTEGER:
		if (put_user(data->value.integer.min, &data32->value.integer.min) ||
		    put_user(data->value.integer.max, &data32->value.integer.max) ||
		    put_user(data->value.integer.step, &data32->value.integer.step))
			return -EFAULT;
		break;
	case SNDRV_CTL_ELEM_TYPE_INTEGER64:
		if (copy_to_user(&data32->value.integer64,
				 &data->value.integer64,
				 sizeof(data->value.integer64)))
			return -EFAULT;
		break;
	case SNDRV_CTL_ELEM_TYPE_ENUMERATED:
		if (copy_to_user(&data32->value.enumerated,
				 &data->value.enumerated,
				 sizeof(data->value.enumerated)))
			return -EFAULT;
		break;
	default:
		break;
	}
	return 0;
}

/* read / write */
struct snd_ctl_elem_value32 {
	struct snd_ctl_elem_id id;
	unsigned int indirect;	/* bit-field causes misalignment */
        union {
		s32 integer[128];
		unsigned char data[512];
#ifndef CONFIG_X86_64
		s64 integer64[64];
#endif
        } value;
        unsigned char reserved[128];
};

#ifdef CONFIG_X86_X32_ABI
/* x32 has a different alignment for 64bit values from ia32 */
struct snd_ctl_elem_value_x32 {
	struct snd_ctl_elem_id id;
	unsigned int indirect;	/* bit-field causes misalignment */
	union {
		s32 integer[128];
		unsigned char data[512];
		s64 integer64[64];
	} value;
	unsigned char reserved[128];
};
#endif /* CONFIG_X86_X32_ABI */

/* get the value type and count of the control */
static int get_ctl_type(struct snd_card *card, struct snd_ctl_elem_id *id,
			int *countp)
{
	struct snd_kcontrol *kctl;
	struct snd_ctl_elem_info *info __free(kfree) = NULL;
	int err;

	guard(rwsem_read)(&card->controls_rwsem);
	kctl = snd_ctl_find_id_locked(card, id);
	if (!kctl)
		return -ENOENT;
	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	info->id = *id;
	err = kctl->info(kctl, info);
	if (err >= 0) {
		err = info->type;
		*countp = info->count;
	}
	return err;
}

static int get_elem_size(snd_ctl_elem_type_t type, int count)
{
	switch (type) {
	case SNDRV_CTL_ELEM_TYPE_INTEGER64:
		return sizeof(s64) * count;
	case SNDRV_CTL_ELEM_TYPE_ENUMERATED:
		return sizeof(int) * count;
	case SNDRV_CTL_ELEM_TYPE_BYTES:
		return 512;
	case SNDRV_CTL_ELEM_TYPE_IEC958:
		return sizeof(struct snd_aes_iec958);
	default:
		return -1;
	}
}

static int copy_ctl_value_from_user(struct snd_card *card,
				    struct snd_ctl_elem_value *data,
				    void __user *userdata,
				    void __user *valuep,
				    int *typep, int *countp)
{
	struct snd_ctl_elem_value32 __user *data32 = userdata;
	int i, type, size;
	int count;
	unsigned int indirect;

	if (copy_from_user(&data->id, &data32->id, sizeof(data->id)))
		return -EFAULT;
	if (get_user(indirect, &data32->indirect))
		return -EFAULT;
	if (indirect)
		return -EINVAL;
	type = get_ctl_type(card, &data->id, &count);
	if (type < 0)
		return type;

	if (type == (__force int)SNDRV_CTL_ELEM_TYPE_BOOLEAN ||
	    type == (__force int)SNDRV_CTL_ELEM_TYPE_INTEGER) {
		for (i = 0; i < count; i++) {
			s32 __user *intp = valuep;
			int val;
			if (get_user(val, &intp[i]))
				return -EFAULT;
			data->value.integer.value[i] = val;
		}
	} else {
		size = get_elem_size((__force snd_ctl_elem_type_t)type, count);
		if (size < 0) {
			dev_err(card->dev, "snd_ioctl32_ctl_elem_value: unknown type %d\n", type);
			return -EINVAL;
		}
		if (copy_from_user(data->value.bytes.data, valuep, size))
			return -EFAULT;
	}

	*typep = type;
	*countp = count;
	return 0;
}

/* restore the value to 32bit */
static int copy_ctl_value_to_user(void __user *userdata,
				  void __user *valuep,
				  struct snd_ctl_elem_value *data,
				  int type, int count)
{
	struct snd_ctl_elem_value32 __user *data32 = userdata;
	int i, size;

	if (type == (__force int)SNDRV_CTL_ELEM_TYPE_BOOLEAN ||
	    type == (__force int)SNDRV_CTL_ELEM_TYPE_INTEGER) {
		for (i = 0; i < count; i++) {
			s32 __user *intp = valuep;
			int val;
			val = data->value.integer.value[i];
			if (put_user(val, &intp[i]))
				return -EFAULT;
		}
	} else {
		size = get_elem_size((__force snd_ctl_elem_type_t)type, count);
		if (copy_to_user(valuep, data->value.bytes.data, size))
			return -EFAULT;
	}
	if (copy_to_user(&data32->id, &data->id, sizeof(data32->id)))
		return -EFAULT;
	return 0;
}

static int __ctl_elem_read_user(struct snd_card *card,
				void __user *userdata, void __user *valuep)
{
	struct snd_ctl_elem_value *data __free(kfree) = NULL;
	int err, type, count;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	err = copy_ctl_value_from_user(card, data, userdata, valuep,
				       &type, &count);
	if (err < 0)
		return err;

	err = snd_ctl_elem_read(card, data);
	if (err < 0)
		return err;
	return copy_ctl_value_to_user(userdata, valuep, data, type, count);
}

static int ctl_elem_read_user(struct snd_card *card,
			      void __user *userdata, void __user *valuep)
{
	int err;

	err = snd_power_ref_and_wait(card);
	if (err < 0)
		return err;
	err = __ctl_elem_read_user(card, userdata, valuep);
	snd_power_unref(card);
	return err;
}

static int __ctl_elem_write_user(struct snd_ctl_file *file,
				 void __user *userdata, void __user *valuep)
{
	struct snd_ctl_elem_value *data __free(kfree) = NULL;
	struct snd_card *card = file->card;
	int err, type, count;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	err = copy_ctl_value_from_user(card, data, userdata, valuep,
				       &type, &count);
	if (err < 0)
		return err;

	err = snd_ctl_elem_write(card, file, data);
	if (err < 0)
		return err;
	return copy_ctl_value_to_user(userdata, valuep, data, type, count);
}

static int ctl_elem_write_user(struct snd_ctl_file *file,
			       void __user *userdata, void __user *valuep)
{
	struct snd_card *card = file->card;
	int err;

	err = snd_power_ref_and_wait(card);
	if (err < 0)
		return err;
	err = __ctl_elem_write_user(file, userdata, valuep);
	snd_power_unref(card);
	return err;
}

static int snd_ctl_elem_read_user_compat(struct snd_card *card,
					 struct snd_ctl_elem_value32 __user *data32)
{
	return ctl_elem_read_user(card, data32, &data32->value);
}

static int snd_ctl_elem_write_user_compat(struct snd_ctl_file *file,
					  struct snd_ctl_elem_value32 __user *data32)
{
	return ctl_elem_write_user(file, data32, &data32->value);
}

#ifdef CONFIG_X86_X32_ABI
static int snd_ctl_elem_read_user_x32(struct snd_card *card,
				      struct snd_ctl_elem_value_x32 __user *data32)
{
	return ctl_elem_read_user(card, data32, &data32->value);
}

static int snd_ctl_elem_write_user_x32(struct snd_ctl_file *file,
				       struct snd_ctl_elem_value_x32 __user *data32)
{
	return ctl_elem_write_user(file, data32, &data32->value);
}
#endif /* CONFIG_X86_X32_ABI */

/* add or replace a user control */
static int snd_ctl_elem_add_compat(struct snd_ctl_file *file,
				   struct snd_ctl_elem_info32 __user *data32,
				   int replace)
{
	struct snd_ctl_elem_info *data __free(kfree) = NULL;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (! data)
		return -ENOMEM;

	/* id, type, access, count */ \
	if (copy_from_user(&data->id, &data32->id, sizeof(data->id)) ||
	    copy_from_user(&data->type, &data32->type, 3 * sizeof(u32)))
		return -EFAULT;
	if (get_user(data->owner, &data32->owner))
		return -EFAULT;
	switch (data->type) {
	case SNDRV_CTL_ELEM_TYPE_BOOLEAN:
	case SNDRV_CTL_ELEM_TYPE_INTEGER:
		if (get_user(data->value.integer.min, &data32->value.integer.min) ||
		    get_user(data->value.integer.max, &data32->value.integer.max) ||
		    get_user(data->value.integer.step, &data32->value.integer.step))
			return -EFAULT;
		break;
	case SNDRV_CTL_ELEM_TYPE_INTEGER64:
		if (copy_from_user(&data->value.integer64,
				   &data32->value.integer64,
				   sizeof(data->value.integer64)))
			return -EFAULT;
		break;
	case SNDRV_CTL_ELEM_TYPE_ENUMERATED:
		if (copy_from_user(&data->value.enumerated,
				   &data32->value.enumerated,
				   sizeof(data->value.enumerated)))
			return -EFAULT;
		data->value.enumerated.names_ptr =
			(uintptr_t)compat_ptr(data->value.enumerated.names_ptr);
		break;
	default:
		break;
	}
	return snd_ctl_elem_add(file, data, replace);
}  

enum {
	SNDRV_CTL_IOCTL_ELEM_LIST32 = _IOWR('U', 0x10, struct snd_ctl_elem_list32),
	SNDRV_CTL_IOCTL_ELEM_INFO32 = _IOWR('U', 0x11, struct snd_ctl_elem_info32),
	SNDRV_CTL_IOCTL_ELEM_READ32 = _IOWR('U', 0x12, struct snd_ctl_elem_value32),
	SNDRV_CTL_IOCTL_ELEM_WRITE32 = _IOWR('U', 0x13, struct snd_ctl_elem_value32),
	SNDRV_CTL_IOCTL_ELEM_ADD32 = _IOWR('U', 0x17, struct snd_ctl_elem_info32),
	SNDRV_CTL_IOCTL_ELEM_REPLACE32 = _IOWR('U', 0x18, struct snd_ctl_elem_info32),
#ifdef CONFIG_X86_X32_ABI
	SNDRV_CTL_IOCTL_ELEM_READ_X32 = _IOWR('U', 0x12, struct snd_ctl_elem_value_x32),
	SNDRV_CTL_IOCTL_ELEM_WRITE_X32 = _IOWR('U', 0x13, struct snd_ctl_elem_value_x32),
#endif /* CONFIG_X86_X32_ABI */
};

static inline long snd_ctl_ioctl_compat(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct snd_ctl_file *ctl;
	struct snd_kctl_ioctl *p;
	void __user *argp = compat_ptr(arg);
	int err;

	ctl = file->private_data;
	if (snd_BUG_ON(!ctl || !ctl->card))
		return -ENXIO;

	switch (cmd) {
	case SNDRV_CTL_IOCTL_PVERSION:
	case SNDRV_CTL_IOCTL_CARD_INFO:
	case SNDRV_CTL_IOCTL_SUBSCRIBE_EVENTS:
	case SNDRV_CTL_IOCTL_POWER:
	case SNDRV_CTL_IOCTL_POWER_STATE:
	case SNDRV_CTL_IOCTL_ELEM_LOCK:
	case SNDRV_CTL_IOCTL_ELEM_UNLOCK:
	case SNDRV_CTL_IOCTL_ELEM_REMOVE:
	case SNDRV_CTL_IOCTL_TLV_READ:
	case SNDRV_CTL_IOCTL_TLV_WRITE:
	case SNDRV_CTL_IOCTL_TLV_COMMAND:
		return snd_ctl_ioctl(file, cmd, (unsigned long)argp);
	case SNDRV_CTL_IOCTL_ELEM_LIST32:
		return snd_ctl_elem_list_compat(ctl->card, argp);
	case SNDRV_CTL_IOCTL_ELEM_INFO32:
		return snd_ctl_elem_info_compat(ctl, argp);
	case SNDRV_CTL_IOCTL_ELEM_READ32:
		return snd_ctl_elem_read_user_compat(ctl->card, argp);
	case SNDRV_CTL_IOCTL_ELEM_WRITE32:
		return snd_ctl_elem_write_user_compat(ctl, argp);
	case SNDRV_CTL_IOCTL_ELEM_ADD32:
		return snd_ctl_elem_add_compat(ctl, argp, 0);
	case SNDRV_CTL_IOCTL_ELEM_REPLACE32:
		return snd_ctl_elem_add_compat(ctl, argp, 1);
#ifdef CONFIG_X86_X32_ABI
	case SNDRV_CTL_IOCTL_ELEM_READ_X32:
		return snd_ctl_elem_read_user_x32(ctl->card, argp);
	case SNDRV_CTL_IOCTL_ELEM_WRITE_X32:
		return snd_ctl_elem_write_user_x32(ctl, argp);
#endif /* CONFIG_X86_X32_ABI */
	}

	guard(rwsem_read)(&snd_ioctl_rwsem);
	list_for_each_entry(p, &snd_control_compat_ioctls, list) {
		if (p->fioctl) {
			err = p->fioctl(ctl->card, ctl, cmd, arg);
			if (err != -ENOIOCTLCMD)
				return err;
		}
	}
	return -ENOIOCTLCMD;
}
