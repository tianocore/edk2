from CommonCtypes import *   
from VfrFormPkg import *

# Ifr related Info -> ctypes obj
#　conditional Info
# Structure Info
        
        
class VfrTreeNode():
    def __init__(self, Opcode: int=None) -> None:
        
        self.OpCode = Opcode
        self.Data = None # save class or bytes
        self.Condition = None
        self.Expression = None
        self.Parent = None
        self.Child = []
        self.NextRel = None # needed?
        self.LastRel = None # needed?
    
    def HasCondition(self) ->bool:
        if self.Condition == None:
            return False
        else:
            return True
        
    
    def HasChild(self) -> bool:
        if self.Child == []:
            return False
        else:
            return True

    def isFinalChild(self) -> bool:
        ParTree = self.Parent
        if ParTree:
            if ParTree.Child[-1] == self:
                return True
        return False
        

    def insertChild(self, NewNode, pos: int=None) -> None:
        if len(self.Child) == 0:
            self.Child.append(NewNode)
        else:
            if not pos:
                LastTree = self.Child[-1]
                self.Child.append(NewNode)
                LastTree.NextRel = NewNode
                NewNode.LastRel = LastTree
            else:
                NewNode.NextRel = self.Child[pos-1].NextRel
                NewNode.LastRel = self.Child[pos].LastRel
                self.Child[pos-1].NextRel = NewNode
                self.Child[pos].LastRel = NewNode
                self.Child.insert(pos, NewNode)
        NewNode.Parent = self
        if self.Condition != None and NewNode.Condition != None:
            NewNode.Condition = self.Condition + ' | ' + NewNode.Condition
        elif self.Condition != None and NewNode.Condition == None:
            NewNode.Condition = self.Condition
            

    # lastNode.insertRel(newNode)
    def insertRel(self, newNode) -> None:
        if self.Parent:
            parentTree = self.Parent
            new_index = parentTree.Child.index(self) + 1
            parentTree.Child.insert(new_index, newNode)
        self.NextRel = newNode
        newNode.LastRel = self
        

    def deleteNode(self, deletekey: str) -> None:
        FindStatus, DeleteTree = self.FindNode(deletekey)
        if FindStatus:
            parentTree = DeleteTree.Parent
            lastTree = DeleteTree.LastRel
            nextTree = DeleteTree.NextRel
            if parentTree:
                index = parentTree.Child.index(DeleteTree)
                del parentTree.Child[index]
            if lastTree and nextTree:
                lastTree.NextRel = nextTree
                nextTree.LastRel = lastTree
            elif lastTree:
                lastTree.NextRel = None
            elif nextTree:
                nextTree.LastRel = None
            return DeleteTree
        else:
            print('Could not find the target tree')
            return None
        
    