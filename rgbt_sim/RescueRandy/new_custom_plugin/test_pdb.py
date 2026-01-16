import pdb
def add(x,y):
    sum = x+y
    return sum

if __name__ == "__main__": 
    x = int (input("Num1: "))
    y = int (input("Num2: "))
    pdb.set_trace()
    z = add (x, y)
    print(z)