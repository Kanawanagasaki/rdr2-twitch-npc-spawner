namespace Rdr2.TwitchNpcSpawner.Data;

public class TwitchApiResponse<T> where T : class
{
    public int StatusCode { get; set; }
    public T[] data { get; init; } = Array.Empty<T>();

    public bool IsSuccessStatusCode => 200 <= StatusCode && StatusCode < 300;
}
