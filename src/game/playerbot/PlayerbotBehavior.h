#include <iostream>
#include <list>
#include <vector>
#include <stack>
#include <initializer_list>
#include <string>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <sstream>

class BehaviourTree {
public:
	class Node {  // This class represents each node in the behaviour tree.
	public:
		virtual bool run() = 0;
	};

	class CompositeNode : public Node {  //  This type of Node follows the Composite Pattern, containing a list of other Nodes.
	private:
		std::vector<Node*> children;
	public:
		const std::vector<Node*>& getChildren() const { return children; }
		void addChild(Node* child) { children.emplace_back(child); }
		void addChildren(std::initializer_list<Node*>&& newChildren) { for (Node* child : newChildren) addChild(child); }
		template <typename CONTAINER>
		void addChildren(const CONTAINER& newChildren) { for (Node* child : newChildren) addChild(child); }
	protected:
		std::vector<Node*> childrenShuffled() const { std::vector<Node*> temp = children;  std::random_shuffle(temp.begin(), temp.end());  return temp; }
	};

	class Selector : public CompositeNode {
	public:
		virtual bool run() override {
			for (Node* child : getChildren()) {  // The generic Selector implementation
				if (child->run())  // If one child succeeds, the entire operation run() succeeds.  Failure only results if all children fail.
					return true;
			}
			return false;  // All children failed so the entire run() operation fails.
		}
	};

	class RandomSelector : public CompositeNode {  // RandomSelector operates as a Selector, but in a random order instead of from first child to last child (adds more unpredictability to the behaviour when a there is no clear preferred order of execution).
	public:
		virtual bool run() override {
			for (Node* child : childrenShuffled()) {  // The order is shuffled
				if (child->run())
					return true;
			}
			return false;
		}
	};

	class Sequence : public CompositeNode {
	public:
		virtual bool run() override {
			for (Node* child : getChildren()) {  // The generic Sequence implementation.
				if (!child->run())  // If one child fails, then enter operation run() fails.  Success only results if all children succeed.
					return false;
			}
			return true;  // All children suceeded, so the entire run() operation succeeds.
		}
	};

	class DecoratorNode : public Node {  // Function is either to transform the result it receives from its child node's status, to terminate the child, or repeat processing of the child, depending on the type of decorator node.
	private:
		Node* child;  // Only one child allowed
	protected:
		Node* getChild() const { return child; }
	public:
		void setChild(Node* newChild) { child = newChild; }
	};

	class Root : public DecoratorNode {
	private:
		friend class BehaviourTree;
		virtual bool run() override { return getChild()->run(); }
	};

	class Inverter : public DecoratorNode {  // Inverts the result of the child. A child fails and it will return success to its parent, or a child succeeds and it will return failure to the parent.
	private:
		virtual bool run() override { return !getChild()->run(); }
	};

	class Succeeder : public DecoratorNode {  // A succeeder will always return success, irrespective of what the child node actually returned. These are useful in cases where you want to process a branch of a tree where a failure is expected or anticipated, but you don’t want to abandon processing of a sequence that branch sits on.
	private:
		virtual bool run() override { getChild()->run();  return true; }
	};

	class Failer : public DecoratorNode {  // The opposite of a Succeeder, always returning fail.  Note that this can be achieved also by using an Inverter and setting its child to a Succeeder.
	private:
		virtual bool run() override { getChild()->run();  return false; }
	};

	class Repeater : public DecoratorNode {  // A repeater will reprocess its child node each time its child returns a result. These are often used at the very base of the tree, to make the tree to run continuously. Repeaters may optionally run their children a set number of times before returning to their parent.
	private:
		int numRepeats;
		static const int NOT_FOUND = -1;
		Repeater(int num = NOT_FOUND) : numRepeats(num) {}  // By default, never terminate.
		virtual bool run() override {
			if (numRepeats == NOT_FOUND)
				while (true) getChild()->run();
			else {
				for (int i = 0; i < numRepeats - 1; i++)
					getChild()->run();
				return getChild()->run();
			}
		}
	};

	class RepeatUntilFail : public DecoratorNode {  // Like a repeater, these decorators will continue to reprocess their child. That is until the child finally returns a failure, at which point the repeater will return success to its parent.
	private:
		virtual bool run() override {
			while (getChild()->run()) {}
			return true;
		}
	};

	template <typename T>
	class StackNode : public Node {
	protected:
		std::stack<T*>& stack;  // Must be reference to a stack to work.
		StackNode(std::stack<T*>& s) : stack(s) {}
	};

	template <typename T>
	class PushToStack : public StackNode<T> {  // Specific type of leaf (hence has no child).
	private:
		T*& item;
	public:
		PushToStack(T*& t, std::stack<T*>& s) : StackNode<T>(s), item(t) {}
	private:
		virtual bool run() override {
			this->stack.push(item);
			return true;
		}
	};

	template <typename T>
	class GetStack : public StackNode<T> {  // Specific type of leaf (hence has no child).
	private:
		const std::stack<T*>& obtainedStack;
		T* object;
	public:
		GetStack(std::stack<T*>& s, const std::stack<T*>& o, T* t = nullptr) : StackNode<T>(s), obtainedStack(o), object(t) {}
	private:
		virtual bool run() override {
			this->stack = obtainedStack;
			if (object)
				this->stack.push(object);
			return true;
		}
	};

	template <typename T>
	class PopFromStack : public StackNode<T> {  // Specific type of leaf (hence has no child).
	private:
		T*& item;
	public:
		PopFromStack(T*& t, std::stack<T*>& s) : StackNode<T>(s), item(t) {}
	private:
		virtual bool run() override {
			if (this->stack.empty())
				return false;
			item = this->stack.top();
			this->stack.pop();
			return true;
		}
	};

	template <typename T>
	class StackIsEmpty : public StackNode<T> {  // Specific type of leaf (hence has no child).
	public:
		StackIsEmpty(std::stack<T*>& s) : StackNode<T>(s) {}
	private:
		virtual bool run() override {
			return this->stack.empty();
		}
	};

	template <typename T>
	class SetVariable : public BehaviourTree::Node {  // Specific type of leaf (hence has no child).
	private:
		T *&variable, *&object;  // Must use reference to pointer to work correctly.
	public:
		SetVariable(T*& t, T*& obj) : variable(t), object(obj) {}
		virtual bool run() override {
			variable = object;
			return true;
		};
	};

	template <typename T>
	class IsNull : public BehaviourTree::Node {  // Specific type of leaf (hence has no child).
	private:
		T*& object;  // Must use reference to pointer to work correctly.
	public:
		IsNull(T*& t) : object(t) {}
		virtual bool run() override { return !object; }
	};
private:
	Root* root;
public:
	BehaviourTree() : root(new Root) {}
	void setRootChild(Node* rootChild) const { root->setChild(rootChild); }
	bool run() const { return root->run(); }
};