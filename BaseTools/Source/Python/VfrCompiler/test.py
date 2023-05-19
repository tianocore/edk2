import yaml
path = 'C:\\edk2\\Build\\OvmfX64\\DEBUG_VS2019\\X64\\NetworkPkg\\IScsiDxe\\IScsiDxe\\DEBUG\\IScsiConfigVfr.yml'
FileName = path
File = open(FileName, 'r')
#self.YamlDict = yaml.load(f.read(), Loader=yaml.FullLoader)
print(yaml.safe_load(File))
# import yaml

# def load_yaml_file(file_path):
#     with open(file_path, 'r') as file:
#         data = yaml.safe_load(file)
#     return data

# def get_dict_line_numbers(data, start_line=1):
#     line_numbers = {}
#     line_numbers[start_line] = data
#     current_line_number = start_line

#     for key, value in data.items():
#         if isinstance(value, dict):
#             child_line_numbers = get_dict_line_numbers(value, start_line=current_line_number+1)
#             line_numbers.update(child_line_numbers)
#             current_line_number = max(child_line_numbers.keys())

#     return line_numbers

#     def PreProcessYamlDict(self, YamlDict):
#         for Key in YamlDict.keys():
#             if Key == 'condition':
#                 continue
#             Value = YamlDict[Key]
#             if isinstance(Value, list): # value is list
#                 for Item in Value:
#                     if isinstance(Item, dict):
#                         self.PreProcessYamlDict(Item)

#             elif isinstance(Value, dict): # value is dict
#                 self.PreProcessYamlDict(Value)

# Line = 0

# def GetDictLineNumber(self, YamlDict, CurDict):
#     if YamlDict == CurDict:
#         return 1
#     for Key in YamlDict:
#         Value = YamlDict[Key]
#         if isinstance(Value, list): # value is list
#             for Item in Value:
#                 if isinstance(Item, dict):
#                     return Line + self.GetDictLineNumber(Item, CurDict)


# # # 读取 YAML 文件并加载到字典中
# # file_path = path
# # data = load_yaml_file(file_path)

# # # 获取字典与文件行数的对应关系
# # dict_line_numbers = get_dict_line_numbers(data)

# # # 输出每个子字典在文件中的行数
# # for line_number, sub_dict in dict_line_numbers.items():
# #     print(f"行号：{line_number}")
# #     print(f"子字典：{sub_dict}")
# #     print("---")





# # import yaml

# # def dump_dict_to_yaml_file(data, file_path):
# #     with open(file_path, 'w') as file:
# #         yaml.dump(data, file)

# # def get_yaml_file_line_count(file_path):
# #     with open(file_path, 'r') as file:
# #         lines = file.readlines()
# #         return len(lines)

# # # 示例字典
# # data = {
# #     'key1': 'value1',
# #     'key2': 'value2',
# #     'key3': {
# #         'nested_key': 'nested_value'
# #     }
# # }

# # # 将字典转换为 YAML 文件
# # yaml_file_path = 'example.yaml'
# # dump_dict_to_yaml_file(data, yaml_file_path)

# # # 获取 YAML 文件的行数
# # line_count = get_yaml_file_line_count(yaml_file_path)
# # print(f"YAML 文件行数：{line_count}")

# import yaml

# def find_and_extract_subdict(data, search_key):
#     if isinstance(data, dict):
#         if search_key in data:
#             return data[search_key]
#         else:
#             for value in data.values():
#                 result = find_and_extract_subdict(value, search_key)
#                 if result is not None:
#                     return result
#     elif isinstance(data, list):
#         for item in data:
#             result = find_and_extract_subdict(item, search_key)
#             if result is not None:
#                 return result
#     return None

# def remove_after_subdict(data, subdict_key):
#     if isinstance(data, dict):
#         if subdict_key in data:
#             data.clear()
#         else:
#             for value in data.values():
#                 remove_after_subdict(value, subdict_key)
#     elif isinstance(data, list):
#         for item in data:
#             remove_after_subdict(item, subdict_key)

# # 示例字典
# data = {
#     'key1': 'value1',
#     'key2': {
#         'nested_key': 'nested_value',
#         'target_key': 'target_value'
#     },
#     'key3': {
#         'key4': 'value4',
#         'key5': 'value5'
#     }
# }

# # 查找并提取目标子字典
# search_key = 'target_key'
# subdict = find_and_extract_subdict(data, search_key)

# if subdict is not None:
#     # 删除子字典之后的内容
#     remove_after_subdict(data, search_key)

#     # 输出新的字典为 YAML 文件
#     yaml_file_path = 'new_dict.yaml'
#     with open(yaml_file_path, 'w') as file:
#         yaml.dump(data, file)
#     print("新的字典已输出为 YAML 文件。")
# else:
#     print("未找到目标子字典。")
