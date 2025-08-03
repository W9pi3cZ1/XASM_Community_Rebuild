# XASM Community Rebuild

这是一个重构了的 XASM 社区版，旨在为用户提供一个更加简洁、易用的开发环境。

## 编译方法

```bash
make
```

## 使用方法

本项目包含`compiler`和`runner`两部分，分别对应编译器和解释器。

### 编译器

```bash
./compiler -i <input_file> -o <output_file>
```

### 解释器

```bash
./runner -i <input_file>
```

## 特性

- 支持用 \[Tag\] 来标记地址，并可通过 @Tag 来展开（以 4 字节进行展开）
- 实现了稀疏内存块（可实现高地址的简单映射）
- ...

## 开发规范

- 使用 `make format` 来格式化代码
- 尽量使用英文书写代码

## 指令文档

可参考[指令文档](./docs/code_table/code_table.md)
