import torch
from transformers import AutoTokenizer, AutoModel
from modelscope.hub.snapshot_download import snapshot_download
import torch.nn as nn
import os
import shutil

class BertEmbeddingModel(nn.Module):
    def __init__(self, model_dir):
        super(BertEmbeddingModel, self).__init__()
        self.bert = AutoModel.from_pretrained(model_dir)
        
    def forward(self, input_ids, attention_mask, token_type_ids):
        outputs = self.bert(input_ids=input_ids, 
                            attention_mask=attention_mask, 
                            token_type_ids=token_type_ids)
        # 获取 [CLS] 向量
        return outputs.last_hidden_state[:, 0, :]

def convert_to_onnx(model_id, output_dir):
    print(f"正在从 ModelScope 下载模型: {model_id}")
    model_dir = snapshot_download(model_id)
    print(f"模型下载到: {model_dir}")

    print(f"正在加载模型...")
    model = BertEmbeddingModel(model_dir)
    model.eval()
    
    tokenizer = AutoTokenizer.from_pretrained(model_dir)
    
    # 准备虚拟输入
    dummy_text = "这是一个测试"
    inputs = tokenizer(dummy_text, return_tensors="pt", padding='max_length', max_length=128, truncation=True)
    
    input_names = ["input_ids", "attention_mask", "token_type_ids"]
    output_names = ["output"]
    
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        
    output_path = os.path.join(output_dir, "model.onnx")
    print(f"正在导出到 ONNX: {output_path}")
    
    torch.onnx.export(model, 
                      (inputs['input_ids'], inputs['attention_mask'], inputs['token_type_ids']),
                      output_path,
                      input_names=input_names,
                      output_names=output_names,
                      dynamic_axes={
                          'input_ids': {0: 'batch_size'},
                          'attention_mask': {0: 'batch_size'},
                          'token_type_ids': {0: 'batch_size'},
                          'output': {0: 'batch_size'}
                      },
                      opset_version=14)
    
    # 保存词表
    vocab_path = os.path.join(output_dir, "vocab.txt")
    print(f"正在保存词表: {vocab_path}")
    if os.path.exists(os.path.join(model_dir, "vocab.txt")):
        shutil.copy(os.path.join(model_dir, "vocab.txt"), vocab_path)
    else:
        tokenizer.save_vocabulary(output_dir)

if __name__ == "__main__":
    model_id = "iic/nlp_corom_sentence-embedding_chinese-tiny"
    export_dir = "export"
    
    convert_to_onnx(model_id, export_dir)
    print(f"导出完成！模型和词表已保存在 {export_dir} 目录下。")
