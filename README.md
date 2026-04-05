# 我的 tmux 配置

## Feature

- 使用 Alt+F 来代替 Ctrl+B 作为全局功能前缀，靠左手大姆指与食指来按，更方便、快捷
- 增加了基于 Alt 快捷键的窗口新建、切换等操作，操作更快捷
- 自定制了底部状态栏，增加了系统资源实时利用率等

## Installation

```bash
git clone https://github.com/nicky-zs/.tmux.git ~/.tmux
ln -s ~/.tmux/tmux.conf ~/.tmux.conf
cd ~/.tmux && make && sudo make install
```

## Usage

重新进入 tmux 或者重新加载配置：

```bash
tmux source-file ~/.tmux.conf
```

## Development

```bash
# 编译
make

# 运行测试
make test && make run-tests

# 编译优化版本
make release

# 安装到 ~/.local
make PREFIX=~/.local install

# 清理
make clean
```

## Requirements

- tmux >= 2.0
- gcc
- Linux (使用 /proc/stat 和 /proc/meminfo)
