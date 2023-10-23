#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/setup.h>

#if defined(CONFIG_INITRAMFS_IGNORE_SKIP_FLAG) || \
    defined(CONFIG_PROC_CMDLINE_APPEND_ANDROID_FORCE_NORMAL_BOOT)
#define INITRAMFS_STR_FIND "skip_initramf"
#endif

#ifdef CONFIG_INITRAMFS_IGNORE_SKIP_FLAG
#define INITRAMFS_STR_REPLACE "want_initramf"
#define INITRAMFS_STR_LEN (sizeof(INITRAMFS_STR_FIND) - 1)
#endif

#ifdef CONFIG_PROC_CMDLINE_APPEND_ANDROID_FORCE_NORMAL_BOOT
#define ANDROID_FORCE_NORMAL_BOOT_STR "androidboot.force_normal_boot=1"
#endif

static char proc_command_line[COMMAND_LINE_SIZE];

static void proc_command_line_init(void) {
	char *offset_addr;
	char *proc_command_line_tail;

	strcpy(proc_command_line, saved_command_line);

#ifdef CONFIG_INITRAMFS_IGNORE_SKIP_FLAG
	offset_addr = strstr(proc_command_line, INITRAMFS_STR_FIND);
	if (offset_addr)
		memcpy(offset_addr, INITRAMFS_STR_REPLACE, INITRAMFS_STR_LEN);
#endif


#ifdef CONFIG_PROC_CMDLINE_APPEND_ANDROID_FORCE_NORMAL_BOOT
	if (strstr(saved_command_line, INITRAMFS_STR_FIND)) {
		// point proc_command_line_tail to the null terminator of the cmdline
		proc_command_line_tail = proc_command_line + strlen(proc_command_line);
		memcpy(proc_command_line_tail, " ", 1);
		memcpy(proc_command_line_tail + 1, ANDROID_FORCE_NORMAL_BOOT_STR,
                        sizeof(ANDROID_FORCE_NORMAL_BOOT_STR));
	}
#endif
}

static int cmdline_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", proc_command_line);
	return 0;
}

static int cmdline_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cmdline_proc_show, NULL);
}

static const struct file_operations cmdline_proc_fops = {
	.open		= cmdline_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

#ifdef REMOVE_SAFETYNET_FLAGS
static void remove_flag(char *cmd, const char *flag)
{
	char *start_addr, *end_addr;

	/* Ensure all instances of a flag are removed */
	while ((start_addr = strstr(cmd, flag))) {
		end_addr = strchr(start_addr, ' ');
		if (end_addr)
			memmove(start_addr, end_addr + 1, strlen(end_addr));
		else
			*(max(cmd, start_addr - 1)) = '\0';
	}
}

static void remove_safetynet_flags(char *cmd)
{
	remove_flag(cmd, "androidboot.veritymode=");
}
#endif

#if 1

static char *padding = "                ";

static void replace_flag(char *cmd, const char *flag, const char *flag_new)
{
	char *start_addr, *end_addr;

	/* Ensure all instances of a flag are replaced */
	while ((start_addr = strstr(cmd, flag))) {
		end_addr = strchr(start_addr, ' ');
		if (end_addr) {
			if (strlen(flag)<strlen(flag_new)) {
				// xx yy=a zz
				//    ^   ^
				// xx yy=bb zz
				int length_to_copy = strlen( start_addr + (strlen(flag) ) ) + 1; // +1 to copy trailing '/0'
				int length_diff = strlen(flag_new)-strlen(flag);
				memcpy(start_addr+(strlen(flag)+length_diff), start_addr+(strlen(flag)), length_to_copy);
				memcpy(start_addr+(strlen(flag)), padding, length_diff);
			}
			memcpy(start_addr, flag_new, strlen(flag_new));
		}
		else
			*(start_addr - 1) = '\0';
	}
}

static void replace_safetynet_flags(char *cmd)
{
	// WARNING: be aware that you can't replace shorter string with longer ones in the function called here...
	replace_flag(cmd, "androidboot.vbmeta.device_state=unlocked",
			  "androidboot.vbmeta.device_state=locked  ");
	replace_flag(cmd, "androidboot.enable_dm_verity=0",
			  "androidboot.enable_dm_verity=1");
	replace_flag(cmd, "androidboot.secboot=disabled",
			  "androidboot.secboot=enabled ");
	replace_flag(cmd, "androidboot.verifiedbootstate=orange",
			  "androidboot.verifiedbootstate=green ");
#ifndef REMOVE_SAFETYNET_FLAGS
	replace_flag(cmd, "androidboot.veritymode=logging",
			  "androidboot.veritymode=enforcing");
	replace_flag(cmd, "androidboot.veritymode=eio",
			  "androidboot.veritymode=enforcing");
#endif

}
#endif

static int __init proc_cmdline_init(void)
{
	proc_command_line_init();

	proc_create("cmdline", 0, NULL, &cmdline_proc_fops);
	return 0;
}
fs_initcall(proc_cmdline_init);
