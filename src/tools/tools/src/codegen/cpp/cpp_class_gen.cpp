#include <halley/support/exception.h>
#include "cpp_class_gen.h"

using namespace Halley;

CPPClassGenerator::CPPClassGenerator(String name)
	: className(name) 
{
	header = "class " + name + " {";
}

CPPClassGenerator::CPPClassGenerator(String name, String baseClass, MemberAccess inheritanceType, bool isFinal)
	: className(name)
{
	header = "class " + name + (isFinal ? " final" : "") + " : " + toString(inheritanceType) + " " + baseClass + " {";
}

CPPClassGenerator& CPPClassGenerator::addClass(CPPClassGenerator& otherClass)
{
	ensureOK();
	otherClass.writeTo(results[currentAccess], 1);
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addBlankLine()
{
	return addRawLine("");
}

CPPClassGenerator& CPPClassGenerator::addLine(String line)
{
	return addRawLine("\t" + line);
}

CPPClassGenerator& CPPClassGenerator::addRawLine(String line)
{
	ensureOK();
	results[currentAccess].push_back(line);
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addComment(String comment)
{
	ensureOK();
	addRawLine("\t// " + comment);
	return *this;
}

CPPClassGenerator& CPPClassGenerator::setAccessLevel(MemberAccess access)
{
	ensureOK();
	currentAccess = access;
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addMember(const MemberSchema& member)
{
	if (member.access) {
		setAccessLevel(member.access.value());
	}
	addRawLine("\t" + getMemberString(member) + ";");
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addMembers(const Vector<MemberSchema>& members)
{
	for (const auto& m : members) {
		addMember(m);
	}
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addMembers(const Vector<ComponentFieldSchema>& members)
{
	for (const auto& m : members) {
		addMember(m);
	}
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addMethodDeclaration(MethodSchema method)
{
	ensureOK();
	addRawLine("\t" + getMethodSignatureString(method) + ";");
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addMethodDeclarations(const Vector<MethodSchema>& methods)
{
	ensureOK();
	for (auto& m : methods) {
		addMethodDeclaration(m);
	}
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addMethodDefinition(MethodSchema method, String body)
{
	return addMethodDefinition(method, Vector<String>{ body });
}

CPPClassGenerator& CPPClassGenerator::addMethodDefinition(MethodSchema method, const Vector<String>& body)
{
	ensureOK();
	addRawLine("\t" + getMethodSignatureString(method) + " {");
	for (auto& line : body) {
		addRawLine("\t\t" + line);
	}
	addRawLine("\t}");
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addTypeDefinition(String name, String type)
{
	ensureOK();
	addRawLine("\tusing " + name + " = " + type + ";");
	return *this;
}

CPPClassGenerator& CPPClassGenerator::finish()
{
	ensureOK();
	finished = true;
	return *this;
}

CPPClassGenerator& CPPClassGenerator::addDefaultConstructor()
{
	return addCustomConstructor({}, {});
}

CPPClassGenerator& CPPClassGenerator::addConstructor(const Vector<VariableSchema>& variables, bool move)
{
	Vector<VariableSchema> init;
	for (auto& v : variables) {
		init.push_back(v);
		init.back().initialValue = move ? ("std::move(" + v.name + ")") : v.name;
	}
	return addCustomConstructor(variables, init);
}

CPPClassGenerator& CPPClassGenerator::addCustomConstructor(const Vector<VariableSchema>& parameters, const Vector<VariableSchema>& initialization, const Vector<String>& body)
{
	String sig = "\t" + getMethodSignatureString(MethodSchema(TypeSchema(""), parameters, className));

	if (!initialization.empty()) {
		addRawLine(sig);
		bool first = true;
		for (const auto& i: initialization) {
			String prefix = first ? "\t\t: ": "\t\t, ";
			first = false;
			addRawLine(prefix + i.name + "(" + i.initialValue + ")");
		}
		addRawLine("\t{");
	} else {
		addRawLine(sig + " {");
	}

	for (const auto& line : body) {
		addRawLine("\t\t" + line);
	}
	addRawLine("\t}");
	
	return *this;
}

void CPPClassGenerator::writeTo(Vector<String>& out, int nTabs)
{
	if (!finished) {
		throw Exception("Class not finished yet.", HalleyExceptions::Tools);
	}

	String prefix;
	for (int i = 0; i < nTabs; i++) {
		prefix += "\t";
	}

	out.push_back(prefix + header);
	for (auto& access: results) {
		out.push_back(prefix + toString(access.first) + ":");
		for (auto& i : access.second) {
			out.push_back(prefix + i);
		}
	}
	out.push_back(prefix + "};");
}

void CPPClassGenerator::ensureOK() const
{
	if (finished) {
		throw Exception("finish() has already been called!", HalleyExceptions::Tools);
	}
}

String CPPClassGenerator::getTypeString(const TypeSchema& type)
{
	String value;
	if (type.isStatic) {
		value += "static ";
	}
	if (type.isConst) {
		value += "const ";
	}
	if (type.isConstExpr) {
		value += "constexpr ";
	}
	value += type.name;
	return value;
}

String CPPClassGenerator::getVariableString(const VariableSchema& var)
{
	String init = "";
	if (var.initialValue != "") {
		init = " = " + var.initialValue;
	}
	return getTypeString(var.type) + " " + var.name + init;
}

String CPPClassGenerator::getMemberString(const MemberSchema& var)
{
	return getTypeString(var.type) + " " + var.name + var.getValueString();
}

String CPPClassGenerator::getAnonString(const MemberSchema& var)
{
	return getTypeString(var.type) + var.getValueString();
}

String MemberSchema::getValueString(bool initializer) const
{
	String init;
	if (defaultValue.empty()) {
		if (!type.name.endsWith("&")) {
			init = "{}";
		}
	} else {
		bool first = true;
		for (const auto& v: defaultValue) {
			if (first) {
				init += "{ ";
				first = false;
			} else {
				init += ", ";
			}

			if (v.isNumber() || v == "true" || v == "false" || v == "nullptr" || (v.startsWith("\"") && v.endsWith("\"")) || v.contains("::")) {
				init += v;
			} else {
				init += "\"" + v + "\"";
			}
		}

		init += " }";
	}

	if (!initializer && init == "{}") {
		return type.name + "()";
	}
	
	return init;
}

String CPPClassGenerator::getMethodSignatureString(const MethodSchema& method)
{
	Vector<String> args;
	for (auto& a : method.arguments) {
		args.push_back(getVariableString(a));
	}
	String returnType = getTypeString(method.returnType);
	if (returnType != "") {
		returnType += " ";
	}

	return String(method.isFriend ? "friend " : "")
		+ (method.isVirtual ? "virtual " : "")
		+ returnType
		+ method.name
		+ "(" + String::concatList(args, ", ") + ")"
		+ (method.isConst ? " const" : "")
		+ (method.isOverride ? " override" : "")
		+ (method.isFinal ? " final" : "")
		+ (method.isPure ? " = 0" : "");
}
