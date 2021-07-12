class Job:
    """
    docstring goes here
    """
    def __init__(self, num_operations: int, op_chain: list, 
                iter: int, num_items: int, items: list):
        """
        docstring goes here
        """
        self.num_operations = len(op_chain)
        self.op_chain = op_chain
        self.iter = int(iter)
        self.num_items = len(items)
        self.items = items

    def debug_job(self):
        """
        docstring goes here
        """
        print(f"Number of Operations: {self.num_operation}")
        print("Operation Chain:")
        for i in self.op_chain:
            print(f"\tOperation Group: {i}")
        print(f"Iterations: {self.iter}")
        print(f"Number of Items:")
        print(f"Operand List: {self.items}")
        for i in self.items:
            print(f"\tItem Group: {i}")

    