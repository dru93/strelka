// -*- mode: c++; indent-tabs-mode: nil; -*-
//
// Starka
// Copyright (c) 2009-2014 Illumina, Inc.
//
// This software is provided under the terms and conditions of the
// Illumina Open Source Software License 1.
//
// You should have received a copy of the Illumina Open Source
// Software License 1 along with this program. If not, see
// <https://github.com/sequencing/licenses/>
//

///
/// \author Chris Saunders
///

#include "inovo_vcf_locus_info.hh"
#include "inovo_streams.hh"

#include "blt_util/chrom_depth_map.hh"
#include "blt_util/io_util.hh"
#include "htsapi/vcf_util.hh"
#include "htsapi/bam_dumper.hh"

#include <cassert>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>


//#define DEBUG_HEADER

#ifdef DEBUG_HEADER
#include "blt_util/log.hh"
#endif



/// add vcf filter tags shared by all vcf types:
static
void
write_shared_vcf_header_info(
    const denovo_filter_options& opt,
    const denovo_filter_deriv_options& dopt,
    std::ostream& os)
{
    if (dopt.is_max_depth())
    {
        using namespace INOVO_VCF_FILTERS;

        std::ostringstream oss;
        oss << "Locus depth is greater than " << opt.max_depth_factor << "x the mean chromosome depth in the normal sample";
        //oss << "Greater than " << opt.max_depth_factor << "x chromosomal mean depth in Normal sample
        write_vcf_filter(os,get_label(HighDepth),oss.str().c_str());

        const StreamScoper ss(os);
        os << std::fixed << std::setprecision(2);

        for (const auto& val : dopt.chrom_depth)
        {
            const std::string& chrom(val.first);
            const double max_depth(opt.max_depth_factor*val.second);
            os << "##MaxDepth_" << chrom << '=' << max_depth << "\n";
        }
    }
}



inovo_streams::
inovo_streams(
    const inovo_options& opt,
    const inovo_deriv_options& dopt,
    const prog_info& pinfo,
    const bam_header_t* const header,
    const InovoSampleSetSummary& ssi)
    : base_t(opt,pinfo,ssi)
{
    {
         if (opt.is_realigned_read_file)
        {
            assert(false);
        }

    }

    const char* const cmdline(opt.cmdline.c_str());

    std::ofstream* fosptr(new std::ofstream);
    _denovo_osptr.reset(fosptr);
    std::ofstream& fos(*fosptr);
    open_ofstream(pinfo,opt.denovo_filename,"denovo-small-variants",opt.is_clobber,fos);

    if (! opt.dfilter.is_skip_header)
    {
        write_vcf_audit(opt,pinfo,cmdline,header,fos);
        fos << "##content=inovo somatic snv calls\n"
            << "##germlineSnvTheta=" << opt.bsnp_diploid_theta << "\n";

        // INFO:
        fos << "##INFO=<ID=DP,Number=1,Type=Integer,Description=\"Combined depth across samples\">\n";
        fos << "##INFO=<ID=MQ,Number=1,Type=Float,Description=\"RMS Mapping Quality\">\n";
        fos << "##INFO=<ID=MQ0,Number=1,Type=Integer,Description=\"Number of MAPQ == 0 reads covering this record\">\n";

        // FORMAT:
        fos << "##FORMAT=<ID=DP,Number=1,Type=Integer,Description=\"Read depth\">\n";

        // FILTERS:
        {
#if 0
            using namespace INOVO_VCF_FILTERS;
            {
                std::ostringstream oss;
                oss << "Fraction of basecalls filtered at this site in either sample is at or above " << opt.sfilter.snv_max_filtered_basecall_frac;
                write_vcf_filter(fos, get_label(BCNoise), oss.str().c_str());
            }
#endif
        }

        write_shared_vcf_header_info(opt.dfilter,dopt.dfilter,fos);

        fos << vcf_col_label() << "\tFORMAT";
        {
            const SampleInfoManager& si(opt.alignFileOpt.alignmentSampleInfo);
            for (unsigned sampleIndex(0); sampleIndex<si.size(); ++sampleIndex)
            {
                fos << "\t" << INOVO_SAMPLETYPE::get_label(si.getSampleInfo(sampleIndex).stype);
            }
        }
        fos << "\n";
    }
#if 0
    if (opt.is_somatic_callable())
    {
        std::ofstream* fosptr(new std::ofstream);
        _somatic_callable_osptr.reset(fosptr);
        std::ofstream& fos(*fosptr);

        open_ofstream(pinfo,opt.somatic_callable_filename,"somatic-callable-regions",opt.is_clobber,fos);

        if (! opt.sfilter.is_skip_header)
        {
            fos << "track name=\"StrelkaCallableSites\"\t"
                << "description=\"Sites with sufficient information to call somatic alleles at 10% frequency or greater.\"\n";
        }
    }
#endif
}
