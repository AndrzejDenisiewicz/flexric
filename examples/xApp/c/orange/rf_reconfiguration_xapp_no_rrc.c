/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*
 * The generic xApp database contains an optional RC-statistics decoder for
 * NR RRC measurement reports.  This KPM-first executable does not consume RC
 * indications, and the checkout does not ship the generated NR RRC decoder.
 * Keep the optional path linkable while making an accidental use fail closed.
 */
#include <stddef.h>
#include <stdint.h>

#include "../../../../src/lib/e2ap/v3_01/ie/asn/asn_random_fill.h"
#include "../../../../src/lib/e2ap/v3_01/ie/asn/asn_application.h"

__attribute__((weak)) asn_TYPE_descriptor_t asn_DEF_NR_UL_DCCH_Message = {0};

__attribute__((weak)) asn_dec_rval_t
uper_decode(const asn_codec_ctx_t* opt_codec_ctx,
            const asn_TYPE_descriptor_t* td,
            void** struct_ptr,
            const void* buffer,
            size_t size,
            int skip_bits,
            int unused_bits)
{
  (void)opt_codec_ctx;
  (void)td;
  (void)struct_ptr;
  (void)buffer;
  (void)size;
  (void)skip_bits;
  (void)unused_bits;
  asn_dec_rval_t result = {RC_FAIL, 0};
  return result;
}