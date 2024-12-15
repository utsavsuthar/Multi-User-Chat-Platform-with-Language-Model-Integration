import time
import torch
import sys
from transformers import AutoTokenizer, AutoModelForCausalLM

def generate_text(prompt,user_id):
    try:
        # start = time.time()
        # Load pre-trained GPT-2 model and tokenizer
        #model_name = "gpt2"
        model_name = "openai-community/gpt2" 
        tokenizer = AutoTokenizer.from_pretrained(model_name)
        model = AutoModelForCausalLM.from_pretrained(model_name)

        # Set device to GPU if available
        device = "cuda" if torch.cuda.is_available() else "cpu"
        model.to(device)
        # print(device)

     
        max_length=200
        input_ids = tokenizer.encode(prompt, return_tensors="pt").to(device)
        output = model.generate(input_ids, max_length=max_length, pad_token_id=tokenizer.eos_token_id, no_repeat_ngram_size=1, num_return_sequences=1, top_p=0.9, temperature=0.7, do_sample=True)
        generated_text = tokenizer.decode(output[0], skip_special_tokens=True)
       
        start_index = generated_text.find('gpt2bot>')
        
# Extract the substring starting from the position after "Chatbot Answer >"
        response = generated_text[start_index+len('gpt2bot>'):].strip()
        output_file = f"{user_id}.txt"
        with open(output_file, "w") as file:
            file.write(response)
        # return generated_text
    except Exception as e:
        # Handle any exceptions that occur during the process
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 gpt_2_gen.py <prompt>")
        sys.exit(1)
    filename = f'{sys.argv[1]}.txt'
    with open(filename,"r") as file:
        question = file.read()
    user_id = sys.argv[1]

    prompt = f"""You are a Chatbot capable of answering questions for airline services. Please respond the following user question posed by the user to the best of your knowledge and do not generate any follow up questions.
    User > {question}
    gpt2bot>"""
    generate_text(prompt, user_id)
