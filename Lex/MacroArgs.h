//===--- MacroArgs.h - Formal argument info for Macros ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the MacroArgs interface.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LEX_MACROARGS_H
#define LLVM_CLANG_LEX_MACROARGS_H

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/ArrayRef.h"
#include <vector>

namespace clang
{
	class MacroInfo;
	class Preprocessor;
	class Token;
	class SourceLocation;

	/// MacroArgs - An instance of this class captures information about
	/// the formal arguments specified to a function-like macro invocation.
	// 所有的MacroArgs被组织成一个链表，每个MacroArgs都有一个指针指向下一个
	//MacroArgs对象 这个指针就是ArgCache
	class MacroArgs
	{
		/// NumUnexpArgTokens - The number of raw, unexpanded tokens for the
		/// arguments.  All of the actual argument tokens are allocated immediately
		/// after the MacroArgs object in memory.  This is all of the arguments
		/// concatenated together, with 'EOF' markers at the end of each argument.
		unsigned NumUnexpArgTokens;

		/// VarargsElided - True if this is a C99 style varargs macro invocation and
		/// there was no argument specified for the "..." argument.  If the argument
		/// was specified (even empty) or this isn't a C99 style varargs function, or
		/// if in strict mode and the C99 varargs macro had only a ... argument, this
		/// is false.
		bool VarargsElided;

		/// PreExpArgTokens - Pre-expanded tokens for arguments that need them.  Empty
		/// if not yet computed.  This includes the EOF marker at the end of the
		/// stream.
		std::vector<std::vector<Token> > PreExpArgTokens;

		/// StringifiedArgs - This contains arguments in 'stringified' form.  If the
		/// stringified form of an argument has not yet been computed, this is empty.
		std::vector<Token> StringifiedArgs;

		/// ArgCache - This is a linked list of MacroArgs objects that the
		/// Preprocessor owns which we use to avoid thrashing malloc/free.
		// 这个是链表的next指针啦，诡异的是这个是free list的指针，意思是对于无效了的
		//MacroArgs 我们并不释放其空间，而是记录这些空闲的为一个链表
		//当需要构造新MacroArgs时，首先从这个空闲链表中找到满足要求的最小内存区，然后构造
		//如果没有找到这个最小内存区，就malloc
		MacroArgs *ArgCache;

		MacroArgs(unsigned NumToks, bool varargsElided)
			: NumUnexpArgTokens(NumToks), VarargsElided(varargsElided),
			ArgCache(nullptr)
		{}
		~MacroArgs() = default;

	public:
		/// MacroArgs ctor function - Create a new MacroArgs object with the specified
		/// macro and argument info.
		static MacroArgs *create(const MacroInfo *MI,
			ArrayRef<Token> UnexpArgTokens,
			bool VarargsElided, Preprocessor &PP);

		/// destroy - Destroy and deallocate the memory for this object.
		///
		void destroy(Preprocessor &PP);

		/// ArgNeedsPreexpansion - If we can prove that the argument won't be affected
		/// by pre-expansion, return false.  Otherwise, conservatively return true.
		bool ArgNeedsPreexpansion(const Token *ArgTok, Preprocessor &PP) const;

		/// getUnexpArgument - Return a pointer to the first token of the unexpanded
		/// token list for the specified formal.
		///
		const Token *getUnexpArgument(unsigned Arg) const;

		/// getArgLength - Given a pointer to an expanded or unexpanded argument,
		/// return the number of tokens, not counting the EOF, that make up the
		/// argument.
		static unsigned getArgLength(const Token *ArgPtr);

		/// getPreExpArgument - Return the pre-expanded form of the specified
		/// argument.
		const std::vector<Token> &
			getPreExpArgument(unsigned Arg, const MacroInfo *MI, Preprocessor &PP);

		/// getStringifiedArgument - Compute, cache, and return the specified argument
		/// that has been 'stringified' as required by the # operator.
		const Token &getStringifiedArgument(unsigned ArgNo, Preprocessor &PP,
			SourceLocation ExpansionLocStart,
			SourceLocation ExpansionLocEnd);

		/// getNumArguments - Return the number of arguments passed into this macro
		/// invocation.
		unsigned getNumArguments() const
		{
			return NumUnexpArgTokens;
		}


		/// isVarargsElidedUse - Return true if this is a C99 style varargs macro
		/// invocation and there was no argument specified for the "..." argument.  If
		/// the argument was specified (even empty) or this isn't a C99 style varargs
		/// function, or if in strict mode and the C99 varargs macro had only a ...
		/// argument, this returns false.
		bool isVarargsElidedUse() const
		{
			return VarargsElided;
		}

		/// StringifyArgument - Implement C99 6.10.3.2p2, converting a sequence of
		/// tokens into the literal string token that should be produced by the C #
		/// preprocessor operator.  If Charify is true, then it should be turned into
		/// a character literal for the Microsoft charize (#@) extension.
		///
		static Token StringifyArgument(const Token *ArgToks,
			Preprocessor &PP, bool Charify,
			SourceLocation ExpansionLocStart,
			SourceLocation ExpansionLocEnd);


		/// deallocate - This should only be called by the Preprocessor when managing
		/// its freelist.
		MacroArgs *deallocate();
	};

}  // end namespace clang

#endif
