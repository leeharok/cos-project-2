import requests
import random
import json

def main():
    lst = []
    base = 5
    for _ in range(1000):
        noise = random.randint(-3, 3)
        num = base + noise
        lst.append(num)
        base += 5

    training = lst[:800]
    testing = lst[800:]

    # By using the requests module, please write the code following the instructions below:
    # 1) Initialize a model "my_model" with the "lstm" algorithm, the dimension 1, and the index 0
    uri = "http://127.0.0.1:5556/my_model"
    data = {}
    data["algorithm"] = "lstm"
    data["dimension"] = 1
    data["index"] = 0
    js = json.dumps(data)
    response = requests.post(uri, json=js)

    # 2) Check if the model you generated is listed in the available models (print it!)
    uri = "http://127.0.0.1:5556/"
    response = requests.get(uri)
    print (response.json())

    # 3) Get the information about the model you generated (print it!)
    uri = "http://127.0.0.1:5556/my_model"
    response = requests.get(uri)
    print (response.json())
    

    # 4) Add all the training instance to the AI module


    # 5) Generate the model based on the training dataset


    # 6) Add all the testing instance to the AI module


    # 7) Get the prediction result from the AI module



if __name__ == "__main__":
    main()
