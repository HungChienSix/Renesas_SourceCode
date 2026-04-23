/* V1.0 LittleFS Port */

#ifndef LFS_PORT_H
#define LFS_PORT_H

#include "lfs.h"

/* LittleFS port层初始化：初始化SD卡、挂载文件系统 */
int  lfs_port_init(void);

/* LittleFS 启动计数测试 */
void lfs_test(void);

/* LittleFS 实例句柄 */
extern lfs_t lfs_sdcard;

/* LittleFS 配置 */
extern struct lfs_config lfs_cfg;

/* LittleFS 文件句柄 */
extern lfs_file_t lfs_file_sdcard;

#endif /* LFS_PORT_H */
