#include "outputstringstream.h"
#include "printf.h"
#include "wasmmalloc.h"

OutputStringStream::OutputStringStream():m_buffer{0},m_buffersize(256),m_pos(0)/*,m_fill(' ')*/,m_width(-1),m_precision(-1)
{
    m_buffer=new char[m_buffersize];
}

OutputStringStream::~OutputStringStream()
{
    delete [] m_buffer;
}

void OutputStringStream::Clear()
{
    m_buffer[0]='\0';
    m_pos=0;
}

char *OutputStringStream::Buffer()
{
    return m_buffer;
}

int16_t OutputStringStream::TextLength() const
{
    return m_pos;
}

void OutputStringStream::Concat(const char *str)
{
    if(str)
    {
        int16_t i=0;
        if(m_pos>=(m_buffersize-1))
        {
            ExpandBuffer();
        }
        while(m_pos<(m_buffersize-1) && str[i])
        {
            m_buffer[m_pos++]=str[i++];
            if(m_pos>=(m_buffersize-1))
            {
                ExpandBuffer();
            }
        }
        m_buffer[m_pos]='\0';
    }
}

void OutputStringStream::ExpandBuffer()
{
    char *current=m_buffer;
    m_buffersize+=256;
    m_buffer=new char[m_buffersize];
    m_pos=0;
    Concat(current);
    delete [] current;
}

/*
void OutputStringStream::SetFill(const char fill)
{
    m_fill=fill;
}
*/

void OutputStringStream::SetWidth(const int32_t width)
{
    m_width=width;
}

void OutputStringStream::SetPrecision(const int32_t precision)
{
    m_precision=precision;
}

OutputStringStream &OutputStringStream::operator<<(const char *str)
{
    Concat(str);
    return *this;
}

OutputStringStream &OutputStringStream::operator<<(const char c)
{
    if(m_pos>=(m_buffersize-1))
    {
        ExpandBuffer();
    }
    if(m_pos<(m_buffersize-1))
    {
        m_buffer[m_pos++]=c;
    }
    m_buffer[m_pos]='\0';
    return *this;
}

OutputStringStream &OutputStringStream::operator<<(const uint32_t val)
{
    return *this << static_cast<uint64_t>(val);
}

OutputStringStream &OutputStringStream::operator<<(const int32_t val)
{
    return *this << static_cast<int64_t>(val);
}

OutputStringStream &OutputStringStream::operator<<(const uint64_t val)
{
    char format[8]="%llu";
    if(m_width>=0)
    {
        snprintf(format,8,"%%%lillu",m_width);
    }

    char buff[32];
    snprintf(buff,31,format,val);
    buff[31]='\0';
    Concat(buff);

    return *this;
}

OutputStringStream &OutputStringStream::operator<<(const int64_t val)
{
    char format[8]="%lli";
    if(m_width>=0)
    {
        snprintf(format,8,"%%%lilli",m_width);
    }

    char buff[32];
    snprintf(buff,31,format,val);
    buff[31]='\0';
    Concat(buff);

    return *this;
}

OutputStringStream &OutputStringStream::operator<<(const float val)
{
    char format[8]="%f";

    if(m_precision>=0)
    {
        snprintf(format,8,"%%.%lif",m_precision);
    }

    char buff[32];
    snprintf(buff,31,format,val);
    buff[31]='\0';
    Concat(buff);

    return *this;
}
