# 示例

* 查看 Lua API 各页面底部的代码片段，以及本仓库中的[教程](../guides/creating-a-lua-mod.md)。
* Hogwarts Legacy 的 mod 开发使用 UE4SS 的 Lua API 作为其主要逻辑 mod 的基础。这个[网站](https://modding.wiki/en/hogwartslegacy/developers/luaexamples)包含了部分 mod 的示例代码。
    * 你可以在这个页面中搜索感兴趣的代码，当搜索命中文本时，网页中折叠的区块会自动展开。
* [Palworld modding wiki](https://pwmodding.wiki/docs/category/lua-modding) 中有一些不错的文档和示例，适合初学者学习如何开发 Lua mod。
* 在 GitHub 上搜索任意调用了命名足够独特的 UE4SS API 函数的 Lua 代码，搜索时排除 UE4SS 仓库本身：
    * https://github.com/search?q=language%3ALua+StaticFindObject+NOT+repo%3AUE4SS-RE%2FRE-UE4SS&type=code
    * https://github.com/search?q=language%3ALua+FindFirstOf+NOT+repo%3AUE4SS-RE%2FRE-UE4SS&type=code
    * 以此类推，只要查询中指定了 `language:Lua` 即可。
