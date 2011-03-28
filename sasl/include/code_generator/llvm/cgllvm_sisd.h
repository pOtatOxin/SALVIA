#ifndef SASL_CODE_GENERATOR_LLVM_CGLLVM_SISD_H
#define SASL_CODE_GENERATOR_LLVM_CGLLVM_SISD_H

#include <sasl/include/code_generator/llvm/cgllvm_impl.h>

namespace llvm{
	class Constant;
}

BEGIN_NS_SASL_CODE_GENERATOR();

// Code generation for SISD( Single Instruction Single Data )
class cgllvm_sisd: public cgllvm_impl{

protected:
	// Override node_ctxt of cgllvm_impl
	template <typename NodeT >
	cgllvm_sctxt* node_ctxt( boost::shared_ptr<NodeT> const& v, bool create_if_need = false ){
		return cgllvm_impl::node_ctxt<NodeT, cgllvm_sctxt>(v, create_if_need);
	}
	cgllvm_sctxt* node_ctxt( sasl::syntax_tree::node&, bool create_if_need = false );

	// Get zero filled value of any type.
	llvm::Constant* zero_value( boost::shared_ptr<sasl::syntax_tree::type_specifier> );
};

END_NS_SASL_CODE_GENERATOR();

#endif