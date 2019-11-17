#pragma once
#include <ossia/audio/audio_parameter.hpp>
#include <ossia/audio/drwav_handle.hpp>
#include <ossia/dataflow/graph_node.hpp>
#include <ossia/dataflow/port.hpp>
#include <ossia/dataflow/nodes/sound.hpp>
#include <ossia/dataflow/audio_stretch_mode.hpp>
#include <ossia/detail/pod_vector.hpp>
#include <ossia/dataflow/nodes/media.hpp>

#include <type_traits>

namespace ossia::nodes
{

class sound_mmap final : public ossia::sound_node
{
public:
  sound_mmap()
  {
    m_outlets.push_back(&audio_out);
  }

  ~sound_mmap()
  {
  }

  void set_start(std::size_t v)
  {
    start = v;
  }

  void set_upmix(std::size_t v)
  {
    upmix = v;
  }

  void set_sound(drwav_handle hdl)
  {
    using namespace snd;
    m_handle = std::move(hdl);
    if(m_handle)
    {
      switch(m_handle.translatedFormatTag())
      {
        case DR_WAVE_FORMAT_PCM:
        {
          switch(m_handle.bitsPerSample())
          {
            case 16:
              m_converter = read_s16;
              break;
            case 24:
              m_converter = read_s24;
              break;
            case 32:
              m_converter = read_s32;
              break;
          }
          break;
        }
        case DR_WAVE_FORMAT_IEEE_FLOAT:
        {
          switch(m_handle.bitsPerSample())
          {
            case 32:
              m_converter = read_f32;
              break;
            case 64:
              m_converter = read_f64;
              break;
          }
          break;
        }
        default:
          m_converter = nullptr;
          break;
      }
    }
    m_resampler.reset(0_tv, m_mode, m_handle.channels(), m_handle.sampleRate());
  }

  void set_native_tempo(double v)
  {
    tempo = v;
  }

  void set_stretch_mode(ossia::audio_stretch_mode mode)
  {
    if(m_mode != mode)
    {
      m_mode = mode;
      m_resampler.reset(0_tv, m_mode, channels(), m_handle.sampleRate());
    }
  }

  void reset_resampler(time_value date) override
  {
    m_resampler.reset(date, m_mode, channels(), m_handle.sampleRate());
  }

  void fetch_audio(int64_t start, int64_t samples_to_write, double** audio_array) noexcept
  {
    const int channels = this->channels();
    const int file_duration = this->duration();

    m_resampleBuffer.resize(channels);
    for(auto& buf : m_resampleBuffer)
      buf.resize(samples_to_write);

    ossia::mutable_audio_span<float> source(channels);

    void* frame_data = alloca(sizeof(double) * samples_to_write * channels);
    if(m_loops)
    {
      for(int k = 0; k < samples_to_write; k++)
      {
        // TODO add a special case if [0; samples_to_write] don't loop around
        int pos =  this->m_start_offset_samples + ((start + k) % this->m_loop_duration_samples);
        if(pos >= file_duration)
        {
          for(int i = 0; i < channels; i++)
            audio_array[i][k] = 0;
          continue;
        }

        const bool ok = this->m_handle.seek_to_pcm_frame(pos);
        if(!ok)
        {
          for(int i = 0; i < channels; i++)
            audio_array[i][k] = 0;
          continue;
        }

        const int max = 1;
        const auto count = this->m_handle.read_pcm_frames(max, frame_data);
        if(count >= 0)
        {
          for(int i = 0; i < channels; i++)
            source[i] = gsl::span(m_resampleBuffer[i].data() + k, count);
          m_converter(source, frame_data, count);
        }
        else
        {
          for(int i = 0; i < channels; i++)
            audio_array[i][k] = 0;
        }
      }
    }
    else
    {
      for(int i = 0; i < channels; i++)
      {
        source[i] = gsl::span(m_resampleBuffer[i].data(), samples_to_write);
      }

      const bool ok = this->m_handle.seek_to_pcm_frame(start + m_start_offset_samples);
      if(!ok)
      {
        for(int i = 0; i < channels; i++)
          for(int k = 0; k < samples_to_write; k++)
            audio_array[i][k] = 0;
        return;
      }

      const auto count = this->m_handle.read_pcm_frames(samples_to_write, frame_data);
      m_converter(source, frame_data, count);
      for(int i = 0; i < channels; i++)
        for(int k = count; k < samples_to_write; k++)
          audio_array[i][k] = 0;
    }

    for(int i = 0; i < channels; i++)
      std::copy_n(m_resampleBuffer[i].data(), samples_to_write, audio_array[i]);
  }

  void fetch_audio(int64_t start, int64_t samples_to_write, float** audio_array) noexcept
  {
    const int channels = this->channels();
    const int file_duration = this->duration();

    ossia::mutable_audio_span<float> source(channels);

    void* frame_data = alloca(sizeof(double) * samples_to_write * channels);
    if(m_loops)
    {
      for(int k = 0; k < samples_to_write; k++)
      {
        // TODO add a special case if [0; samples_to_write] don't loop around
        int pos =  this->m_start_offset_samples + ((start + k) % this->m_loop_duration_samples);
        if(pos >= file_duration)
        {
          for(int i = 0; i < channels; i++)
            audio_array[i][k] = 0;
          continue;
        }

        const bool ok = this->m_handle.seek_to_pcm_frame(pos);
        if(!ok)
        {
          for(int i = 0; i < channels; i++)
            audio_array[i][k] = 0;
          continue;
        }

        const int max = 1;
        const auto count = this->m_handle.read_pcm_frames(max, frame_data);
        if(count >= 0)
        {
          for(int i = 0; i < channels; i++)
            source[i] = gsl::span(audio_array[i] + k, count);
          m_converter(source, frame_data, count);
        }
        else
        {
          for(int i = 0; i < channels; i++)
            audio_array[i][k] = 0;
        }
      }
    }
    else
    {
      for(int i = 0; i < channels; i++)
      {
        source[i] = gsl::span(audio_array[i], samples_to_write);
      }

      const bool ok = this->m_handle.seek_to_pcm_frame(start + m_start_offset_samples);
      if(!ok)
      {
        for(int i = 0; i < channels; i++)
          for(int k = 0; k < samples_to_write; k++)
            audio_array[i][k] = 0;
        return;
      }

      const auto count = this->m_handle.read_pcm_frames(samples_to_write, frame_data);
      m_converter(source, frame_data, count);
      for(int i = 0; i < channels; i++)
        for(int k = count; k < samples_to_write; k++)
          audio_array[i][k] = 0;
    }
  }

  void
  run(const ossia::token_request& t, ossia::exec_state_facade e) noexcept override
  {
    if(!m_handle)
      return;

    const auto channels = m_handle.channels();
    const auto len = m_handle.totalPCMFrameCount();

    ossia::audio_port& ap = *audio_out.data.target<ossia::audio_port>();
    ap.samples.resize(channels);

    const auto [samples_to_read, samples_to_write] = snd::sample_info(e.bufferSize(), e.modelToSamples(), t);
    if(samples_to_write == 0)
      return;

    assert(samples_to_write > 0);

    const auto samples_offset = t.physical_start(e.modelToSamples());
    if (t.speed > 0)
    {
      if(t.prev_date < m_prev_date)
      {
        reset_resampler(t.prev_date);
      }

      // Allocate some space
      frame_data = nullptr;

      for (std::size_t chan = 0; chan < channels; chan++)
      {
        ap.samples[chan].resize(e.bufferSize());
      }

      m_loop_duration_samples = m_loop_duration.impl * e.modelToSamples();
      m_start_offset_samples = m_start_offset.impl * e.modelToSamples();

      // resample
      m_resampler.run(*this, t, e, channels, len, samples_to_read, samples_to_write, samples_offset, ap);

      for (std::size_t chan = 0; chan < channels; chan++)
      {
        // fade
        snd::do_fade(t.start_discontinuous, t.end_discontinuous, ap.samples[chan],
                     samples_offset, samples_to_read);
      }

      ossia::snd::perform_upmix(this->upmix, channels, ap);
      ossia::snd::perform_start_offset(this->start, ap);

      m_prev_date = t.date;
    }
    else
    {
      /* TODO */
    }
  }

  std::size_t channels() const
  {
    return m_handle ? m_handle.channels() : 0;
  }
  std::size_t duration() const
  {
    return m_handle ? m_handle.totalPCMFrameCount(): 0;
  }

private:
  drwav_handle m_handle{};

  ossia::outlet audio_out{ossia::audio_port{}};

  std::size_t start{};
  std::size_t upmix{};
  double tempo{};

  using read_fn_t = void(*)(ossia::mutable_audio_span<float>& ap, void* data, int64_t samples);
  read_fn_t m_converter{};
  std::vector<char> m_safetyBuffer;
  std::vector<std::vector<float>> m_resampleBuffer;
  void* frame_data{};

  resampler m_resampler;
  audio_stretch_mode m_mode{};

  time_value m_prev_date{};

  int64_t m_loop_duration_samples{};
  int64_t m_start_offset_samples{};
};

}
