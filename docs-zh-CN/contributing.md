# 为 RE-UE4SS 做贡献

感谢你抽出时间为本项目做贡献！

我们鼓励并重视所有类型的贡献。本指南介绍了为项目做贡献的流程和规范。

## 目录

- [报告问题](#报告问题)
- [Pull Request 规范](#pull-request-规范)
- [开发流程](#开发流程)
- [代码风格规范](#代码风格规范)
- [提交信息规范](#提交信息规范)
- [升级规范](#升级规范)
- [许可证](#许可证)

## 报告问题

在提问之前，最好先搜索现有的 [issue](https://github.com/UE4SS-RE/RE-UE4SS/issues) 并阅读已有的[文档](https://docs.ue4ss.com/)。

如果你仍然需要进一步的说明：

- 使用正确的模板提交一个 [issue](https://github.com/UE4SS-RE/RE-UE4SS/issues)
- 尽可能详细地描述你遇到的情况
- 提供项目与平台的版本信息（操作系统等）
- 如果问题与构建相关，请提供工具链信息（编译器版本等）
- 只要 `UE4SS.log` 可用就请一并提供，即使它看起来没有用处

> **安全问题**：切勿在 issue 跟踪器中报告与安全相关的问题、漏洞或缺陷（包括敏感信息）。请将敏感缺陷通过邮件发送至 <security@ue4ss.com>。

## Pull Request 规范

### 通用要求

1. 提交 PR 之前，请确保你的改动已在以下环境测试过：
   - 至少一个 4.11 到 4.25 之间的虚幻引擎版本（即 UProperty 被 FProperty 取代之前的版本）
   - 至少一个 4.25 之后的版本，最好为 UE 5.1 或更新版本

2. 所有 PR 必须包含：
   - 对改动的清晰描述
   - 复现所修复问题的步骤（如适用）
   - 能够复现该问题或演示该新功能的代码
   - 任何相关的测试用例或示例
   - 在 [Changelog.md](/assets/Changelog.md) 文件中更新相关改动

3. 代码应具有良好的文档，并遵循仓库现有的风格约定。

## 开发流程

1. Fork 本仓库
2. 为你的功能或缺陷修复创建一个新分支
3. 实现你的改动
4. 运行适用于你所做改动的测试
5. 如有必要，更新文档
6. 提交一个 pull request

## 代码风格规范

### UEPseudo 代码

- UEPseudo 中的代码应遵循虚幻引擎的格式与风格约定
- 直接沿用虚幻引擎代码实现时，请保持其原始风格
- 任何相对于原版 UE 代码的偏离或修复都应包裹在以下注释之间：

  ```cpp
  // RE-UE4SS FIX (贡献者缩写或用户名): [为什么需要该修复的说明]
  // 修改后的代码写在这里
  // RE-UE4SS FIX END
  ```

### 主仓库代码

- 主仓库中的所有代码在提交前都应运行过 `clang-format`
- 遵循仓库现有的代码风格以保持一致
- 使用有意义的变量名与函数名
- 为复杂逻辑或非显而易见的实现添加注释

## 提交信息规范

- 我们倾向于对所有提交信息使用 [Conventional Commits](https://www.conventionalcommits.org/) 格式。

  ```
  <type>[optional scope]: <description>

  [optional body]

  [optional footer(s)]
  ```

- 常见类型包括：feat、fix、docs、style、refactor、test、chore
- 如适用，请在提交信息中引用 issue 编号
- 让每次提交专注于单一的逻辑改动

## 升级规范

我们维护一份升级指南，用于记录破坏性变更并帮助用户在不同版本之间迁移。

### 面向贡献者

对 API 做出破坏性变更时：

1. 你必须在 `/docs/upgrade-guide.md` 中记录所有破坏性变更，包括：
   - 对改动内容的清晰描述
   - 做出该改动的原因
   - 展示旧用法与新用法的代码示例
   - 任何特殊的迁移步骤

2. 破坏性变更应遵循以下原则：
   - 尽量减少对现有代码的影响
   - 尽可能在移除前提供弃用警告
   - 考虑向后兼容或过渡性 API
   - 通过版本号清晰地传达变更（遵循语义化版本规范）

3. 提交包含破坏性变更的 PR 时：
   - 在 PR 标题中明确标记 `breaking-change`
   - 在 Changelog 中更新“Breaking Changes”小节
   - 在升级指南中引用该 PR

### 面向 API 使用者

- `/docs/upgrade-guide.md` 文件包含逐版本的迁移说明
- 每个主要和次要版本都有各自的小节，详细说明变更
- 在更新到新版本之前，请务必先阅读升级指南

## 许可证

### RE-UE4SS 许可证

向 RE-UE4SS 做贡献即表示你同意你的贡献将以 [MIT 许可证](https://github.com/UE4SS-RE/RE-UE4SS/blob/main/LICENSE) 进行授权。

### UEPseudo 代码许可

UEPseudo 代码受 Epic Games 的许可条款约束。

感谢你帮助改进 RE-UE4SS！

## 署名

本指南基于 [contributing.md](https://contributing.md/generator) 生成。

> **关于翻译**：本页面为社区中文翻译，内容可能滞后于英文原版。如中英文版本存在差异，请以[英文原版](https://docs.ue4ss.com/dev/contributing.html)为准。
