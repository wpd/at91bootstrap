int ubi_glue_init(struct nand_info *nand);
struct kernel_param;
int ubi_mtd_param_parse(const char *val, struct kernel_param *kp);
int ubi_init(void);
void display_volume_info(void);
