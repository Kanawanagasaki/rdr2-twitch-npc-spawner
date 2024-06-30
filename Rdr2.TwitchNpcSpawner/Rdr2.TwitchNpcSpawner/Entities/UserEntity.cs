namespace Rdr2.TwitchNpcSpawner.Entities;

using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using Microsoft.EntityFrameworkCore;

[Table("users"), Index(nameof(TwitchId))]
public class UserEntity
{
    [Key, DatabaseGenerated(DatabaseGeneratedOption.Identity)]
    public int Id { get; init; }
    public required string TwitchId { get; init; }
    public required string AccessToken { get; set; }
    public required string RefreshToken { get; set; }
    public required string IdToken { get; set; }
}
