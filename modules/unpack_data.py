import struct
from typing import Dict

class Unpack_Data:
    """
    doc string tho?
    """
    def __init__(self):
        pass

    def unpack_results(self, response: bytes) -> Dict:
        """
        doc string tho?
        """
        results: Dict = {}
        task = struct.unpack('!' + ('I' * (len(response)//4)), response)
        status = task[0]
        if status == 0:
            results["Status"] = status
            num_items = task[1]
            results["Num Items"] = num_items
            items = task[2::2]
            results["Items"] = items
            answers = task[3::2]
            results["Answers"] = answers
        elif status == 1:
            results["Status"] = status
        elif status == 2:
            results["Status"] = status
        
        return results

    def print_results(self, results: Dict, job_id: bytes) -> None:
        id = struct.unpack('!I', job_id)
        if results["Status"] == 1:
            print(f"No job with ID {id[0]} found on scheduler...")
            exit()
        elif results["Status"] == 2:
            print(f"Job ID {id[0]} is still being computed, please try again later...")
            exit()
        elif results["Status"] == 0:
            print("SUCCESS")
            print(f"Job ID {id[0]} has {results['Num Items']} of {results['Num Items']} completed")
            for i, j in zip(results['Items'], results['Answers']):
                print(f"Item: {i} ---> Answer: {j}")
            exit()
