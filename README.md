# astream

## 介绍
astream是一款支持用户自定义配置目标文件的流信息，以使能NVMe SSD磁盘的多流（multi-stream）特性，从而延长磁盘寿命的便捷工具。

### 编译安装
- 下载astream源码
    ```
    git clone https://gitee.com/openeuler/astream.git
    ```
- 编译
    ```
    cd src && make && make install
    ```
### 安装验证
`astream`默认安装在`/usr/bin`下，安装后可直接使用`astream`命令，即表示软件安装成功。

## 使用说明

### 流分配规则文件介绍与示例
#### 介绍

流分配规则文件中，每一行表示定义的一条规则，每条规则示例如下： `^/path/xx/undo 1` 

它表示为路径/path/xx/下的所有以`undo`为前缀的文件都分配流信息1。 

#### 示例

如下示例一个具体的MySQL的流分配规则文件。

```
^/data/mysql/data/ib_logfile 2
^/data/mysql/data/ibdata1$ 3
^/data/mysql/data/undo 4
^/data/mysql/data/mysql-bin 5
```

该规则文件定义了如下四条规则：

- 以`/data/mysql/data/ib_logfile`为前缀的文件绝对路径对应的文件配置流信息2

- 以`/data/mysql/data/ibdata1`为文件绝对路径对应的文件配置流信息3

- 以`/data/mysql/data/undo`为前缀的文件绝对路径对应的文件配置流信息4

- 以`/data/mysql/data/mysql-bin`为前缀的文件绝对路径对应的文件配置流信息5
###  命令行参数说明

​	`astream [options]`

| 参数 | 参数含义                                                     | 示例说明                                         |
| ---- | ------------------------------------------------------------ | ------------------------------------------------ |
| -h   | 显示astream使用说明                                          | `astream -h`                                     |
| -l   | 设置astream监控过程的显示日志级别, debug(1), info(2), warn(3), error(4) | `astream -i /path/xx/ -r /path/to/rule.txt -l 2` |
| -i   | 指定当前需要监控的目录，多个目录可以以空格间隔输入           | 该参数配合-r使用, 示例见下                       |
| -r   | 指定监控目录对应的流分配规则文件，传入的个数取决于-i，且互相对应 | `astream -i /path/xx -r rule_file.txt`           |
| stop | 正常停止astream守护进程                                          | astream stop                                     |
### 启动astream守护进程

- 监控单目录 

  `astream -i /path/xx/ -r /path/to/stream_rule1.txt`

- 监控多目录 

  本工具支持同时监控多个目录，意味着每个监控目录都需要传入适配的流分配规则文件，支持同时监控两个目录的工具使用示例如下： 

  `astream -i /path/xx/ /path/yy/ -r /path/to/stream_rule1.txt /path/to/stream_rule2.txt `

  命令中监控两个目录，目录1`/path/xx`，对应的流分配规则文件为`/path/to/stream_rule1.txt`，目录2`/path/yy`，对应的流分配规则文件为`/path/to/stream_rule1`。
### NVMe SSD磁盘的写放大WA计算
- 安装`nvme-cli`软件包
  `yum install nvme-cli`
- 执行`calculate_wa.sh`脚本计算WA
  由于磁盘WA的收益需要长稳测试才能相对明显，因此，我们利用定时器crontab，每隔1小时执行计算当前磁盘的WA，具体操作如下：

  假设脚本位于`/root`下，则执行`crontab -e`，加入定时任务
  `0 */1 * * * bash /root/calculate.sh nvme0`
  其中，若所测的`NVMe`磁盘盘符名为`/dev/nvme0n1`，则定时任务中脚本的参数传入`nvme0`即可，其它依此类推。