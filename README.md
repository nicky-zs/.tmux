# 我的tmux配置

## Feature

- 使用 Alt+F 来代替 Ctrl+B 作为全局功能前缀，靠左手大姆指与食指来按，更方便、快捷
- 增加了基于Alt快捷键的窗口新建、切换等操作，操作更快捷
- 自定制了底部状态栏，增加了系统资源实时利用率等

## Usage

```bash
git clone https://github.com/nicky-zs/.tmux.git ~/.tmux && ln -s ~/.tmux/tmux.conf ~/.tmux.conf
cd .tmux && sh gen_bin.sh
``
